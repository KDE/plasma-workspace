/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *   Copyright 2013 Ivan Cukic <ivan.cukic@kde.org>
 *   Copyright 2013 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "shellcorona.h"

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QQmlContext>
#include <QTimer>
#include <QDBusConnection>

#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <Plasma/Package>
#include <Plasma/PluginLoader>
#include <kactivities/controller.h>
#include <kactivities/consumer.h>
#include <ksycoca.h>
#include <kservicetypetrader.h>
#include <KGlobalAccel>
#include <KAuthorized>
#include <KWindowSystem>
#include <kdeclarative/kdeclarative.h>
#include <kdeclarative/qmlobject.h>
#include <KMessageBox>

#include <KScreen/Config>
#include <kscreen/configmonitor.h>

#include "config-ktexteditor.h" // HAVE_KTEXTEDITOR

#include "alternativeshelper.h"
#include "activity.h"
#include "desktopview.h"
#include "panelview.h"
#include "scripting/desktopscriptengine.h"
#include "plasmaquick/configview.h"
#include "plasmaquick/shellpluginloader.h"
#include "shellmanager.h"
#include "osd.h"

#include "plasmashelladaptor.h"

#include "../lookandfeelaccess/lookandfeelaccess.h"

#ifndef NDEBUG
    #define CHECK_SCREEN_INVARIANTS screenInvariants();
#else
    #define CHECK_SCREEN_INVARIANTS
#endif

static const int s_configSyncDelay = 10000; // 10 seconds

class ShellCorona::Private {
public:
    Private(ShellCorona *corona)
        : q(corona),
          activityController(new KActivities::Controller(q)),
          activityConsumer(new KActivities::Consumer(q)),
          addPanelAction(nullptr),
          addPanelsMenu(nullptr),
          interactiveConsole(nullptr),
          screenConfiguration(nullptr),
          loading(false)
    {
        appConfigSyncTimer.setSingleShot(true);
        appConfigSyncTimer.setInterval(s_configSyncDelay);
        connect(&appConfigSyncTimer, &QTimer::timeout, q, &ShellCorona::syncAppConfig);

        waitingPanelsTimer.setSingleShot(true);
        waitingPanelsTimer.setInterval(250);
        connect(&waitingPanelsTimer, &QTimer::timeout, q, &ShellCorona::createWaitingPanels);

        reconsiderOutputsTimer.setSingleShot(true);
        reconsiderOutputsTimer.setInterval(1000);
        connect(&reconsiderOutputsTimer, &QTimer::timeout, q, &ShellCorona::reconsiderOutputs);
    }

    ShellCorona *q;
    QString shell;
    QList<DesktopView *> views;
    KActivities::Controller *activityController;
    KActivities::Consumer *activityConsumer;
    QHash<const Plasma::Containment *, PanelView *> panelViews;
    KConfigGroup desktopDefaultsConfig;
    QList<Plasma::Containment *> waitingPanels;
    QHash<QString, Activity *> activities;
    QHash<QString, QHash<int, Plasma::Containment *> > desktopContainments;
    QAction *addPanelAction;
    QMenu *addPanelsMenu;
    Plasma::Package lookNFeelPackage;
    QSet<KScreen::Output *> redundantOutputs;
    QList<KDeclarative::QmlObject *> alternativesObjects;
    KDeclarative::QmlObject *interactiveConsole;

    KScreen::Config *screenConfiguration;
    QTimer waitingPanelsTimer;
    QTimer appConfigSyncTimer;
    QTimer reconsiderOutputsTimer;
    bool loading;
};

static QScreen *outputToScreen(KScreen::Output *output)
{
    if (!output) {
        return nullptr;
    }

    foreach(QScreen *screen, QGuiApplication::screens()) {
        if (screen->name() == output->name()) {
            return screen;
        }
    }

    return nullptr;
}

static KScreen::Output *screenToOutput(QScreen* screen, KScreen::Config* config)
{
    foreach(KScreen::Output* output, config->connectedOutputs()) {
        if (screen->name() == output->name()) {
            return output;
        }
    }

    return nullptr;
}

KScreen::Output* ShellCorona::findPrimaryOutput() const
{
    foreach (KScreen::Output* output, d->screenConfiguration->connectedOutputs()) {
        if (output->isPrimary())
            return output;
    }

    return nullptr;
}

ShellCorona::ShellCorona(QObject *parent)
    : Plasma::Corona(parent),
      d(new Private(this))
{
    d->desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package().filePath("defaults")), "Desktop");

    new PlasmaShellAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/PlasmaShell"), this);

    connect(this, &Plasma::Corona::startupCompleted, this,
            []() {
                QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                               QStringLiteral("/KSplash"),
                                               QStringLiteral("org.kde.KSplash"),
                                               QStringLiteral("setStage"));
                ksplashProgressMessage.setArguments(QList<QVariant>() << QStringLiteral("desktop"));
                QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
            });

    // Look for theme config in plasmarc, if it isn't configured, take the theme from the
    // LookAndFeel package, if either is set, change the default theme

    connect(qApp, &QCoreApplication::aboutToQuit, [=]() {
        //saveLayout is a slot but arguments not compatible
        saveLayout();
    });

    const QString themeGroupKey = QStringLiteral("Theme");
    const QString themeNameKey = QStringLiteral("name");

    QString themeName;

    KConfigGroup plasmarc(KSharedConfig::openConfig("plasmarc"), themeGroupKey);
    themeName = plasmarc.readEntry(themeNameKey, themeName);

    if (themeName.isEmpty()) {
        LookAndFeelAccess access;
        KConfigGroup lnfCfg = KConfigGroup(KSharedConfig::openConfig(
                                                access.filePath("defaults")),
                                                "plasmarc"
                                           );
        lnfCfg = KConfigGroup(&lnfCfg, themeGroupKey);
        themeName = lnfCfg.readEntry(themeNameKey, themeName);
    }

    if (!themeName.isEmpty()) {
        Plasma::Theme *t = new Plasma::Theme(this);
        t->setThemeName(themeName);
    }

    connect(this, &ShellCorona::containmentAdded,
            this, &ShellCorona::handleContainmentAdded);

    QAction *dashboardAction = actions()->add<QAction>("show dashboard");
    QObject::connect(dashboardAction, &QAction::triggered,
                     this, &ShellCorona::setDashboardShown);
    dashboardAction->setText(i18n("Show Dashboard"));

    dashboardAction->setAutoRepeat(true);
    dashboardAction->setCheckable(true);
    dashboardAction->setIcon(QIcon::fromTheme("dashboard-show"));
    dashboardAction->setData(Plasma::Types::ControlAction);
    KGlobalAccel::self()->setDefaultShortcut(dashboardAction, QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_F12));
    KGlobalAccel::self()->setShortcut(dashboardAction, QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_F12));


    checkAddPanelAction();
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), this, SLOT(checkAddPanelAction(QStringList)));


    //Activity stuff
    QAction *activityAction = actions()->addAction("manage activities");
    connect(activityAction, &QAction::triggered,
            this, &ShellCorona::toggleActivityManager);
    activityAction->setText(i18n("Activities..."));
    activityAction->setIcon(QIcon::fromTheme("preferences-activities"));
    activityAction->setData(Plasma::Types::ConfigureAction);
    activityAction->setShortcut(QKeySequence("alt+d, alt+a"));
    activityAction->setShortcutContext(Qt::ApplicationShortcut);

    connect(d->activityConsumer, SIGNAL(currentActivityChanged(QString)), this, SLOT(currentActivityChanged(QString)));
    connect(d->activityConsumer, SIGNAL(activityAdded(QString)), this, SLOT(activityAdded(QString)));
    connect(d->activityConsumer, SIGNAL(activityRemoved(QString)), this, SLOT(activityRemoved(QString)));

    new Osd(this);
    d->screenConfiguration = KScreen::Config::current();
}

ShellCorona::~ShellCorona()
{
    qDeleteAll(d->views);
    qDeleteAll(d->panelViews);
    delete d;
}

void ShellCorona::setShell(const QString &shell)
{
    if (d->shell == shell) {
        return;
    }

    d->shell = shell;
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/Shell");
    package.setPath(shell);
    package.setAllowExternalPaths(true);
    setPackage(package);
    d->desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package.filePath("defaults")), "Desktop");

    //FIXME: this would change the runtime platform to a fixed one if available
    // but a different way to load platform specific components is needed beforehand
    // because if we import and use two different components plugin, the second time
    // the import is called it will fail
   /* KConfigGroup cg(KSharedConfig::openConfig(package.filePath("defaults")), "General");
    KDeclarative::KDeclarative::setRuntimePlatform(cg.readEntry("DefaultRuntimePlatform", QStringList()));*/

    unload();

    if (d->activityConsumer->serviceStatus() == KActivities::Consumer::Unknown) {
        connect(d->activityConsumer, SIGNAL(serviceStatusChanged(Consumer::ServiceStatus)), SLOT(load()), Qt::UniqueConnection);
    } else {
        load();
    }
}

QString ShellCorona::shell() const
{
    return d->shell;
}

bool outputLess(KScreen::Output *a, KScreen::Output *b)
{
    return ((a->isEnabled() && !b->isEnabled())
         || (a->isEnabled() == b->isEnabled() && (a->isPrimary() && !b->isPrimary()))
         || (a->isPrimary() == b->isPrimary() && (a->pos().x() < b->pos().x()
         || (a->pos().x() == b->pos().x() && a->pos().y() < b->pos().y()))));
}

static QList<KScreen::Output*> sortOutputs(const QHash<int, KScreen::Output*> &outputs)
{
    QList<KScreen::Output*> ret = outputs.values();
    std::sort(ret.begin(), ret.end(), outputLess);
    return ret;
}

void ShellCorona::load()
{
    if (d->shell.isEmpty() ||
        d->activityConsumer->serviceStatus() == KActivities::Consumer::Unknown) {
        return;
    }

    disconnect(d->activityConsumer, SIGNAL(serviceStatusChanged(Consumer::ServiceStatus)), this, SLOT(load()));

    loadLayout("plasma-" + d->shell + "-appletsrc");

    checkActivities();
    if (containments().isEmpty()) {
        loadDefaultLayout();
        foreach(Plasma::Containment *containment, containments()) {
            containment->setActivity(d->activityConsumer->currentActivity());
        }
    } else {
        processUpdateScripts();
        foreach(Plasma::Containment *containment, containments()) {
            if (containment->formFactor() == Plasma::Types::Horizontal ||
                containment->formFactor() == Plasma::Types::Vertical) {
                if (!d->waitingPanels.contains(containment)) {
                    d->waitingPanels << containment;
                }
            } else {
                //FIXME ideally fix this, or at least document the crap out of it
                int screen = containment->lastScreen();
                if (screen < 0) {
                    screen = d->desktopContainments[containment->activity()].count();
                }
                qDebug() << "inserting...";
                insertContainment(containment->activity(), screen, containment);
            }
        }
    }

    KScreen::ConfigMonitor::instance()->addConfig(d->screenConfiguration);
    //we're not going through the connectedOutputs because we need to connect to all outputs
    for (KScreen::Output *output : sortOutputs(d->screenConfiguration->outputs())) {
        addOutput(output);
    }
    connect(d->screenConfiguration, &KScreen::Config::outputAdded, this, &ShellCorona::addOutput);

    if (!d->waitingPanels.isEmpty()) {
        d->waitingPanelsTimer.start();
    }
}

void ShellCorona::primaryOutputChanged()
{
    if (d->views.isEmpty()) {
        return;
    }

    KScreen::Output* output = findPrimaryOutput();
    if (!output) {
        return;
    }

    QScreen *oldPrimary = d->views[0]->screen();
    QScreen *newPrimary = outputToScreen(output);
    qDebug() << "primary changed!" << oldPrimary->name() << newPrimary->name();

    foreach (DesktopView *view, d->views) {
        if (view->screen() == newPrimary) {
            Q_ASSERT(d->views[0]->screen() != view->screen());

            Q_ASSERT(oldPrimary != newPrimary);
            Q_ASSERT(d->views[0]->screen() == oldPrimary);
            Q_ASSERT(d->views[0]->screen() != newPrimary);
            Q_ASSERT(d->views[0]->geometry() == oldPrimary->geometry());
            qDebug() << "adapting" << newPrimary->geometry() << oldPrimary->geometry();

            view->setScreen(oldPrimary);
            break;
        }
    }

    d->views[0]->setScreen(newPrimary);

    foreach (PanelView *panel, d->panelViews) {
        if (panel->screen() == oldPrimary) {
            panel->setScreen(newPrimary);
        } else if (panel->screen() == newPrimary) {
            panel->setScreen(oldPrimary);
        }
    }

    CHECK_SCREEN_INVARIANTS
}

#ifndef NDEBUG
void ShellCorona::screenInvariants() const
{
    Q_ASSERT(d->views.count() <= QGuiApplication::screens().count());
    QScreen *s = d->views.isEmpty() ? nullptr : d->views[0]->screen();
    KScreen::Output *primaryOutput = findPrimaryOutput();
    if (!s) {
        qWarning() << "error: couldn't find primary output" << primaryOutput;
        return;
    }

    QScreen* ks = outputToScreen(primaryOutput);
    Q_ASSERT(!ks || ks == s || !primaryOutput->isEnabled());

    QSet<QScreen*> screens;
    int i = 0;
    foreach (const DesktopView *view, d->views) {
        QScreen *screen = view->screen();
        Q_ASSERT(!screens.contains(screen));
        Q_ASSERT(!d->redundantOutputs.contains(screenToOutput(screen, d->screenConfiguration)));
        Q_ASSERT(view->fillScreen() || ShellManager::s_forceWindowed);
//         commented out because a different part of the code-base is responsible for this
//         and sometimes is not yet called here.
//         Q_ASSERT(!view->fillScreen() || view->geometry() == screen->geometry());
        Q_ASSERT(view->containment());
        Q_ASSERT(view->containment()->screen() == i);
        Q_ASSERT(view->containment()->lastScreen() == i);
        Q_ASSERT(view->isVisible());

        foreach (const PanelView *panel, panelsForScreen(screen)) {
            Q_ASSERT(panel->containment());
            Q_ASSERT(panel->containment()->screen() == i);
            Q_ASSERT(panel->isVisible());
        }

        screens.insert(screen);
        ++i;
    }

    foreach (KScreen::Output* out, d->redundantOutputs) {
        Q_ASSERT(isOutputRedundant(out));
    }

    if (d->views.isEmpty()) {
        qWarning() << "no screens!!";
    }
}
#endif

void ShellCorona::showAlternativesForApplet(Plasma::Applet *applet)
{
    const QString alternativesQML = package().filePath("appletalternativesui");
    if (alternativesQML.isEmpty()) {
        return;
    }

    KDeclarative::QmlObject *qmlObj = new KDeclarative::QmlObject(this);
    qmlObj->setInitializationDelayed(true);
    qmlObj->setSource(QUrl::fromLocalFile(alternativesQML));

    AlternativesHelper *helper = new AlternativesHelper(applet, qmlObj);
    qmlObj->engine()->rootContext()->setContextProperty("alternativesHelper", helper);

    d->alternativesObjects << qmlObj;
    qmlObj->completeInitialization();
    connect(qmlObj->rootObject(), SIGNAL(visibleChanged(bool)),
            this, SLOT(alternativesVisibilityChanged(bool)));
}

void ShellCorona::alternativesVisibilityChanged(bool visible)
{
    if (visible) {
        return;
    }

    QObject *root = sender();

    QMutableListIterator<KDeclarative::QmlObject *> it(d->alternativesObjects);
    while (it.hasNext()) {
        KDeclarative::QmlObject *obj = it.next();
        if (obj->rootObject() == root) {
            it.remove();
            obj->deleteLater();
        }
    }
}

void ShellCorona::unload()
{
    if (d->shell.isEmpty()) {
        return;
    }

    qDeleteAll(containments());
}

KSharedConfig::Ptr ShellCorona::applicationConfig()
{
    return KSharedConfig::openConfig();
}

void ShellCorona::requestApplicationConfigSync()
{
    d->appConfigSyncTimer.start();
}

void ShellCorona::loadDefaultLayout()
{
    const QString script = package().filePath("defaultlayout");
    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;

        WorkspaceScripting::DesktopScriptEngine scriptEngine(this, true);

        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this,
                [](const QString &msg) {
                    qWarning() << msg;
                });
        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this,
                [](const QString &msg) {
                    qDebug() << msg;
                });
        scriptEngine.evaluateScript(code);
    }
}

void ShellCorona::processUpdateScripts()
{
    WorkspaceScripting::DesktopScriptEngine scriptEngine(this, true);

    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this,
            [](const QString &msg) {
                qWarning() << msg;
            });
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this,
            [](const QString &msg) {
                qDebug() << msg;
            });
    foreach (const QString &script, WorkspaceScripting::ScriptEngine::pendingUpdateScripts(this)) {
        scriptEngine.evaluateScript(script);
    }
}

KActivities::Controller *ShellCorona::activityController()
{
    return d->activityController;
}

int ShellCorona::numScreens() const
{
    return QGuiApplication::screens().count();
}

QRect ShellCorona::screenGeometry(int id) const
{
    if (id >= d->views.count() || id < 0) {
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = outputToScreen(findPrimaryOutput());
        return s ? s->geometry() : QRect();
    }
    return d->views[id]->geometry();
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    if (id >= d->views.count() || id < 0) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = outputToScreen(findPrimaryOutput());
        return s ? s->availableGeometry() : QRegion();
    }
    DesktopView *view = d->views[id];
    const QRect screenGeo(view->geometry());

    QRegion r = view->geometry();
    foreach (const PanelView *v, d->panelViews) {
        if (v->screen() == v->screen() && v->visibilityMode() != PanelView::AutoHide) {
            //if the panel is being moved around, we still want to calculate it from the edge
            r -= v->geometryByDistance(0);
        }
    }
    return r;
}

QRect ShellCorona::availableScreenRect(int id) const
{
    if (id >= d->views.count() || id < 0) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = outputToScreen(findPrimaryOutput());
        return s ? s->availableGeometry() : QRect();
    }

    DesktopView *view = d->views[id];
    const QRect screenGeo(view->geometry());

    QRect r = view->geometry();
    foreach (PanelView *v, d->panelViews) {
        if (v->screen() == view->screen() && v->visibilityMode() != PanelView::AutoHide) {
            switch (v->location()) {
            case Plasma::Types::LeftEdge:
                r.setLeft(r.left() + v->thickness());
                break;
            case Plasma::Types::RightEdge:
                r.setRight(r.right() - v->thickness());
                break;
            case Plasma::Types::TopEdge:
                r.setTop(r.top() + v->thickness());
                break;
            case Plasma::Types::BottomEdge:
                r.setBottom(r.bottom() - v->thickness());
            default:
                break;
            }
        }
    }
    return r;
}

QScreen *ShellCorona::screenForId(int screenId) const
{
    DesktopView *v = d->views.value(screenId);
    return v ? v->screen() : nullptr;
}

void ShellCorona::remove(DesktopView *desktopView)
{
    removeView(d->views.indexOf(desktopView));
}

PanelView *ShellCorona::panelView(Plasma::Containment *containment) const
{
    return d->panelViews.value(containment);
}

///// SLOTS

QList<PanelView *> ShellCorona::panelsForScreen(QScreen *screen) const
{
    QList<PanelView *> ret;
    foreach (PanelView *v, d->panelViews) {
        if (v->screen() == screen) {
            ret += v;
        }
    }
    return ret;
}

void ShellCorona::removeView(int idx)
{
    if (idx < 0 || idx >= d->views.count() || d->views.isEmpty()) {
        return;
    }

    bool panelsAltered = false;

    const QScreen *lastScreen = d->views.last()->screen();
    QMutableHashIterator<const Plasma::Containment *, PanelView *> it(d->panelViews);
    while (it.hasNext()) {
        it.next();
        PanelView *panelView = it.value();

        if (panelView->screen() == lastScreen) {
            d->waitingPanels << panelView->containment();
            it.remove();
            delete panelView;
            panelsAltered = true;
        }
    }

    for (int i = d->views.count() - 2; i >= idx; --i) {
        QScreen *screen = d->views[i + 1]->screen();

        const bool wasVisible = d->views[idx]->isVisible();
        d->views[i]->setScreen(screen);

        if (wasVisible) {
            d->views[idx]->show(); //when destroying the screen, QScreen might have hidden the window
        }

        const QList<PanelView *> panels = panelsForScreen(screen);
        panelsAltered = panelsAltered || !panels.isEmpty();
        foreach (PanelView *p, panels) {
            p->setScreen(screen);
        }
    }

    delete d->views.takeLast();

    if (panelsAltered) {
        emit availableScreenRectChanged();
        emit availableScreenRegionChanged();
    }
}

void ShellCorona::outputEnabledChanged()
{
    addOutput(qobject_cast<KScreen::Output *>(sender()));
}

bool ShellCorona::isOutputRedundant(KScreen::Output *screen) const
{
    const QRect geometry = screen->geometry();

    //FIXME: QScreen doesn't have any idea of "this qscreen is clone of this other one
    //so this ultra inefficient heuristic has to stay until we have a slightly better api
    foreach (const KScreen::Output *s, d->screenConfiguration->connectedOutputs()) {
        if (screen == s || !s->isEnabled() || !s->currentMode()) {
            continue;
        }

        const QRect sGeometry = s->geometry();
        if (sGeometry.contains(geometry, false) &&
            sGeometry.width() > geometry.width() &&
            sGeometry.height() > geometry.height()) {
            return true;
        }
    }

    return false;
}

void ShellCorona::reconsiderOutputs()
{
    if (d->loading) {
        d->reconsiderOutputsTimer.start();
        return;
    }

    foreach (KScreen::Output *out, d->screenConfiguration->connectedOutputs()) {
        if (!out->isEnabled()) {
            continue;
        }

        if (d->redundantOutputs.contains(out)) {
            if (!isOutputRedundant(out)) {
                addOutput(out);
            }
        } else if (isOutputRedundant(out)) {
            QScreen *screen = outputToScreen(out);
            for (int i = 0; i < d->views.count(); ++i) {
                if (d->views[i]->screen() == screen) {
                    removeView(i);
                    break;
                }
            }

            d->redundantOutputs.insert(out);
        }
    }

    CHECK_SCREEN_INVARIANTS
}

void ShellCorona::addOutput(KScreen::Output *output)
{
    if (!output) {
        return;
    }

    connect(output, &KScreen::Output::isEnabledChanged, this, &ShellCorona::outputEnabledChanged, Qt::UniqueConnection);
    connect(output, &KScreen::Output::posChanged, &d->reconsiderOutputsTimer, static_cast<void (QTimer::*)()>(&QTimer::start), Qt::UniqueConnection);
    connect(output, &KScreen::Output::currentModeIdChanged, &d->reconsiderOutputsTimer, static_cast<void (QTimer::*)()>(&QTimer::start), Qt::UniqueConnection);
    connect(output, &KScreen::Output::isPrimaryChanged, this, &ShellCorona::primaryOutputChanged, Qt::UniqueConnection);

    if (!output->isEnabled()) {
        d->redundantOutputs.remove(output);
        d->reconsiderOutputsTimer.start();
        return;
    }

    QScreen *screen = outputToScreen(output);
    Q_ASSERT(screen);

    if (isOutputRedundant(output)) {
        d->redundantOutputs.insert(output);
        return;
    } else {
        d->redundantOutputs.remove(output);
    }

    int insertPosition = 0;
    foreach (DesktopView *view, d->views) {
        KScreen::Output *out = screenToOutput(view->screen(), d->screenConfiguration);
        if (outputLess(output, out)) {
            break;
        }

        insertPosition++;
    }

    QScreen* newScreen = insertScreen(screen, insertPosition);

    DesktopView *view = new DesktopView(this);

    Plasma::Containment *containment = createContainmentForActivity(d->activityController->currentActivity(), d->views.count());
    Q_ASSERT(containment);

    QAction *removeAction = containment->actions()->action("remove");
    if (removeAction) {
        removeAction->deleteLater();
    }

    d->views.append(view);
    d->loading = true;
    view->setContainment(containment);
    d->loading = false;
    view->setScreen(newScreen);
    view->show();

    //need to specifically call the reactToScreenChange, since when the screen is shown it's not yet
    //in the list. We still don't want to have an invisible view added.
    containment->reactToScreenChange();

    //were there any panels for this screen before it popped up?
    if (!d->waitingPanels.isEmpty()) {
        d->waitingPanelsTimer.start();
    }

    emit availableScreenRectChanged();
    emit availableScreenRegionChanged();

    CHECK_SCREEN_INVARIANTS
}

QScreen* ShellCorona::insertScreen(QScreen *screen, int idx)
{
    if (idx == d->views.count()) {
        return screen;
    }

    DesktopView* v = d->views[idx];
    QScreen *oldScreen = v->screen();
    v->setScreen(screen);
    foreach (PanelView *panel, d->panelViews) {
        if (panel->screen() == oldScreen) {
            panel->setScreen(screen);
        }
    }

    return insertScreen(oldScreen, idx+1);
}

Plasma::Containment *ShellCorona::createContainmentForActivity(const QString& activity, int screenNum)
{
    QHash<int, Plasma::Containment *> act = d->desktopContainments.value(activity);
    QHash<int, Plasma::Containment *>::const_iterator it = act.constFind(screenNum);
    if (it != act.constEnd()) {
        return *it;
    }

    QString plugin = "org.kde.desktopcontainment";
    if (d->activities.contains(activity)) {
      //  plugin = d->activities.value(activity)->defaultPlugin();
    }

    Plasma::Containment *containment = containmentForScreen(screenNum, d->desktopDefaultsConfig.readEntry("Containment", plugin), QVariantList());
    Q_ASSERT(containment);

    if (containment) {
        containment->setActivity(activity);
        insertContainment(activity, screenNum, containment);
    }

    return containment;
}

void ShellCorona::createWaitingPanels()
{
    if (d->loading) {
        d->waitingPanelsTimer.start();
        return;
    }

    QList<Plasma::Containment *> stillWaitingPanels;

    foreach (Plasma::Containment *cont, d->waitingPanels) {
        //ignore non existing (yet?) screens
        int requestedScreen = cont->lastScreen();
        if (requestedScreen < 0) {
            ++requestedScreen;
        }

        if (requestedScreen > (d->views.count() - 1)) {
            stillWaitingPanels << cont;
            continue;
        }

        PanelView* panel = new PanelView(this);

        Q_ASSERT(qBound(0, requestedScreen, d->views.count() - 1) == requestedScreen);
        QScreen *screen = d->views[requestedScreen]->screen();

        d->panelViews[cont] = panel;
        d->loading = true;
        panel->setContainment(cont);
        d->loading = false;
        panel->setScreen(screen);
        panel->show();
        cont->reactToScreenChange();

        connect(cont, SIGNAL(destroyed(QObject*)), this, SLOT(containmentDeleted(QObject*)));
    }
    d->waitingPanels = stillWaitingPanels;
    emit availableScreenRectChanged();
    emit availableScreenRegionChanged();
}

void ShellCorona::containmentDeleted(QObject *cont)
{
    d->panelViews.remove(static_cast<Plasma::Containment*>(cont));
    emit availableScreenRectChanged();
    emit availableScreenRegionChanged();
}

void ShellCorona::handleContainmentAdded(Plasma::Containment *c)
{
    connect(c, &Plasma::Containment::showAddWidgetsInterface,
            this, &ShellCorona::toggleWidgetExplorer);
    connect(c, &Plasma::Containment::appletAlternativesRequested,
            this, &ShellCorona::showAlternativesForApplet);
}

void ShellCorona::toggleWidgetExplorer()
{
    const QPoint cursorPos = QCursor::pos();
    foreach (DesktopView *view, d->views) {
        if (view->screen()->geometry().contains(cursorPos)) {
            //The view QML has to provide something to display the widget explorer
            view->rootObject()->metaObject()->invokeMethod(view->rootObject(), "toggleWidgetExplorer", Q_ARG(QVariant, QVariant::fromValue(sender())));
            return;
        }
    }
}

void ShellCorona::toggleActivityManager()
{
    const QPoint cursorPos = QCursor::pos();
    foreach (DesktopView *view, d->views) {
        if (view->screen()->geometry().contains(cursorPos)) {
            //The view QML has to provide something to display the activity explorer
            view->rootObject()->metaObject()->invokeMethod(view->rootObject(), "toggleActivityManager");
            return;
        }
    }
}

void ShellCorona::syncAppConfig()
{
    applicationConfig()->sync();
}

void ShellCorona::setDashboardShown(bool show)
{
    QAction *dashboardAction = actions()->action("show dashboard");

    if (dashboardAction) {
        dashboardAction->setText(show ? i18n("Hide Dashboard") : i18n("Show Dashboard"));
    }

    foreach (DesktopView *view, d->views) {
        view->setDashboardShown(show);
    }
}

void ShellCorona::toggleDashboard()
{
    foreach (DesktopView *view, d->views) {
        view->setDashboardShown(!view->isDashboardShown());
    }
}

void ShellCorona::showInteractiveConsole()
{
    if (KSharedConfig::openConfig()->isImmutable() || !KAuthorized::authorize("plasma-desktop/scripting_console")) {
        delete d->interactiveConsole;
        d->interactiveConsole = 0;
        return;
    }

    if (!d->interactiveConsole) {
        const QString consoleQML = package().filePath("interactiveconsole");
        if (consoleQML.isEmpty()) {
            return;
        }

        d->interactiveConsole = new KDeclarative::QmlObject(this);
        d->interactiveConsole->setInitializationDelayed(true);
        d->interactiveConsole->setSource(QUrl::fromLocalFile(consoleQML));
        d->interactiveConsole->completeInitialization();
        connect(d->interactiveConsole->rootObject(), SIGNAL(visibleChanged(bool)),
                this, SLOT(interactiveConsoleVisibilityChanged(bool)));
    }

    d->interactiveConsole->rootObject()->setProperty("mode", "Desktop");
    d->interactiveConsole->rootObject()->setProperty("visible", true);
}

void ShellCorona::loadScriptInInteractiveConsole(const QString &script)
{
    showInteractiveConsole();
    if (d->interactiveConsole) {
        d->interactiveConsole->rootObject()->setProperty("script", script);
    }
}

void ShellCorona::interactiveConsoleVisibilityChanged(bool visible)
{
    if (!visible) {
        d->interactiveConsole->deleteLater();
        d->interactiveConsole = nullptr;
    }
}

void ShellCorona::checkActivities()
{
    KActivities::Consumer::ServiceStatus status = d->activityController->serviceStatus();
    //qDebug() << "$%$%$#%$%$%Status:" << status;
    if (status != KActivities::Consumer::Running) {
        //panic and give up - better than causing a mess
        qDebug() << "ShellCorona::checkActivities is called whilst activity daemon is still connecting";
        return;
    }

    QStringList existingActivities = d->activityConsumer->activities();
    foreach (const QString &id, existingActivities) {
        activityAdded(id);
    }

    // Checking whether the result we got is valid. Just in case.
    Q_ASSERT_X(!existingActivities.isEmpty(), "isEmpty", "There are no activities, and the service is running");
    Q_ASSERT_X(existingActivities[0] != QStringLiteral("00000000-0000-0000-0000-000000000000"),
            "null uuid", "There is a nulluuid activity present");

    // Killing the unassigned containments
    foreach (Plasma::Containment *cont, containments()) {
        if ((cont->containmentType() == Plasma::Types::DesktopContainment ||
             cont->containmentType() == Plasma::Types::CustomContainment) &&
            !existingActivities.contains(cont->activity())) {
            cont->destroy();
        }
    }
}

void ShellCorona::currentActivityChanged(const QString &newActivity)
{
//     qDebug() << "Activity changed:" << newActivity;

    for (int i = 0; i < d->views.count(); ++i) {
        Plasma::Containment *c = createContainmentForActivity(newActivity, i);

        QAction *removeAction = c->actions()->action("remove");
        if (removeAction) {
            removeAction->deleteLater();
        }
        d->views[i]->setContainment(c);
    }
}

void ShellCorona::activityAdded(const QString &id)
{
    //TODO more sanity checks
    if (d->activities.contains(id)) {
        qWarning() << "Activity added twice" << id;
        return;
    }

    Activity *a = new Activity(id, this);
    d->activities.insert(id, a);
}

void ShellCorona::activityRemoved(const QString &id)
{
    Activity *a = d->activities.take(id);
    a->deleteLater();
}

Activity *ShellCorona::activity(const QString &id)
{
    return d->activities.value(id);
}

void ShellCorona::insertActivity(const QString &id, Activity *activity)
{
    d->activities.insert(id, activity);
    Plasma::Containment *c = createContainmentForActivity(id, d->views.count());
    if (c) {
        c->config().writeEntry("lastScreen", 0);
    }
}

Plasma::Containment *ShellCorona::setContainmentTypeForScreen(int screen, const QString &plugin)
{
    Plasma::Containment *oldContainment = containmentForScreen(screen);

    //no valid containment in given screen, giving up
    if (!oldContainment) {
        return 0;
    }

    if (plugin.isEmpty()) {
        return oldContainment;
    }

    DesktopView *view = 0;
    foreach (DesktopView *v, d->views) {
        if (v->containment() == oldContainment) {
            view = v;
            break;
        }
    }

    //no view? give up
    if (!view) {
        return oldContainment;
    }

    //create a new containment
    Plasma::Containment *newContainment = createContainmentDelayed(plugin);

    //if creation failed or invalid plugin, give up
    if (!newContainment) {
        return oldContainment;
    } else if (!newContainment->pluginInfo().isValid()) {
        newContainment->deleteLater();
        return oldContainment;
    }

    newContainment->setWallpaper(oldContainment->wallpaper());

    //At this point we have a valid new containment from plugin and a view
    //copy all configuration groups (excluded applets)
    KConfigGroup oldCg = oldContainment->config();

    //newCg *HAS* to be from a KSharedConfig, because some KConfigSkeleton will need to be synced
    //this makes the configscheme work
    KConfigGroup newCg(KSharedConfig::openConfig(oldCg.config()->name()), "Containments");
    newCg = KConfigGroup(&newCg, QString::number(newContainment->id()));

    //this makes containment->config() work, is a separate thing from its configscheme
    KConfigGroup newCg2 = newContainment->config();

    foreach (const QString &group, oldCg.groupList()) {
        if (group != "Applets") {
            KConfigGroup subGroup(&oldCg, group);
            KConfigGroup newSubGroup(&newCg, group);
            subGroup.copyTo(&newSubGroup);

            KConfigGroup newSubGroup2(&newCg2, group);
            subGroup.copyTo(&newSubGroup2);
        }
    }

    newContainment->init();
    newContainment->restore(newCg);
    newContainment->updateConstraints(Plasma::Types::StartupCompletedConstraint);
    newContainment->save(newCg);
    requestConfigSync();
    newContainment->flushPendingConstraintsEvents();
    emit containmentAdded(newContainment);

    //Move the applets
    foreach (Plasma::Applet *applet, oldContainment->applets()) {
        newContainment->addApplet(applet);
    }

    //remove the "remove" action
    QAction *removeAction = newContainment->actions()->action("remove");
    if (removeAction) {
        removeAction->deleteLater();
    }
    view->setContainment(newContainment);
    newContainment->setActivity(oldContainment->activity());
    d->desktopContainments.remove(oldContainment->activity());
    insertContainment(oldContainment->activity(), screen, newContainment);

    //removing the focus from the item that is going to be destroyed
    //fixes a crash
    //delayout the destruction of the old containment fixes another crash
    view->rootObject()->setFocus(true, Qt::MouseFocusReason);
    QTimer::singleShot(2500, oldContainment, SLOT(destroy()));

    return newContainment;
}

void ShellCorona::checkAddPanelAction(const QStringList &sycocaChanges)
{
    if (!sycocaChanges.isEmpty() && !sycocaChanges.contains("services")) {
        return;
    }

    delete d->addPanelAction;
    d->addPanelAction = 0;

    delete d->addPanelsMenu;
    d->addPanelsMenu = 0;

    KPluginInfo::List panelContainmentPlugins = Plasma::PluginLoader::listContainmentsOfType("Panel");
    const QString constraint = QString("[X-Plasma-Shell] == '%1' and 'panel' ~in [X-Plasma-ContainmentCategories]")
                                      .arg(qApp->applicationName());
    KService::List templates = KServiceTypeTrader::self()->query("Plasma/LayoutTemplate", constraint);

    if (panelContainmentPlugins.count() + templates.count() == 1) {
        d->addPanelAction = new QAction(i18n("Add Panel"), this);
        d->addPanelAction->setData(Plasma::Types::AddAction);
        connect(d->addPanelAction, SIGNAL(triggered(bool)), this, SLOT(addPanel()));
    } else if (!panelContainmentPlugins.isEmpty()) {
        d->addPanelsMenu = new QMenu;
        d->addPanelAction = d->addPanelsMenu->menuAction();
        d->addPanelAction->setText(i18n("Add Panel"));
        d->addPanelAction->setData(Plasma::Types::AddAction);
        qDebug() << "populateAddPanelsMenu" << panelContainmentPlugins.count();
        connect(d->addPanelsMenu, SIGNAL(aboutToShow()), this, SLOT(populateAddPanelsMenu()));
        connect(d->addPanelsMenu, SIGNAL(triggered(QAction*)), this, SLOT(addPanel(QAction*)));
    }

    if (d->addPanelAction) {
        d->addPanelAction->setIcon(QIcon::fromTheme("list-add"));
        actions()->addAction("add panel", d->addPanelAction);
    }
}

void ShellCorona::populateAddPanelsMenu()
{
    d->addPanelsMenu->clear();
    const KPluginInfo emptyInfo;

    KPluginInfo::List panelContainmentPlugins = Plasma::PluginLoader::listContainmentsOfType("Panel");
    QMap<QString, QPair<KPluginInfo, KService::Ptr> > sorted;
    foreach (const KPluginInfo &plugin, panelContainmentPlugins) {
        sorted.insert(plugin.name(), qMakePair(plugin, KService::Ptr(0)));
    }

    const QString constraint = QString("[X-Plasma-Shell] == '%1' and 'panel' in [X-Plasma-ContainmentCategories]")
                                      .arg(qApp->applicationName());
    KService::List templates = KServiceTypeTrader::self()->query("Plasma/LayoutTemplate", constraint);
    foreach (const KService::Ptr &service, templates) {
        sorted.insert(service->name(), qMakePair(emptyInfo, service));
    }

    QMapIterator<QString, QPair<KPluginInfo, KService::Ptr> > it(sorted);
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/LayoutTemplate");
    while (it.hasNext()) {
        it.next();
        QPair<KPluginInfo, KService::Ptr> pair = it.value();
        if (pair.first.isValid()) {
            KPluginInfo plugin = pair.first;
            QAction *action = d->addPanelsMenu->addAction(i18n("Empty %1", plugin.name()));
            if (!plugin.icon().isEmpty()) {
                action->setIcon(QIcon::fromTheme(plugin.icon()));
            }

            action->setData(plugin.pluginName());
        } else {
            //FIXME: proper names
            KPluginInfo info(pair.second);
            const QString path = QStandardPaths::locate(QStandardPaths::GenericDataLocation, package.defaultPackageRoot() + info.pluginName() + "/metadata.desktop");
            if (!path.isEmpty()) {
                package.setPath(info.pluginName());
                const QString scriptFile = package.filePath("mainscript");
                if (!scriptFile.isEmpty()) {
                    QAction *action = d->addPanelsMenu->addAction(info.name());
                    action->setData(QString::fromLatin1("plasma-desktop-template:%1").arg(scriptFile));
                }
            }
        }
    }
}

void ShellCorona::addPanel()
{
    KPluginInfo::List panelPlugins = Plasma::PluginLoader::listContainmentsOfType("Panel");

    if (!panelPlugins.isEmpty()) {
        addPanel(panelPlugins.first().pluginName());
    }
}

void ShellCorona::addPanel(QAction *action)
{
    const QString plugin = action->data().toString();
    if (plugin.startsWith("plasma-desktop-template:")) {
        WorkspaceScripting::DesktopScriptEngine scriptEngine(this, true);

        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this,
                [](const QString &msg) {
                    qWarning() << msg;
                });
        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this,
                [](const QString &msg) {
                    qDebug() << msg;
                });
        const QString scriptFile = plugin.right(plugin.length() - qstrlen("plasma-desktop-template:"));
        QFile file(scriptFile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << i18n("Unable to load script file: %1", scriptFile);
            return;
        }

        QString script = file.readAll();
        if (script.isEmpty()) {
            // qDebug() << "script is empty";
            return;
        }

        scriptEngine.evaluateScript(script, scriptFile);
    } else if (!plugin.isEmpty()) {
        addPanel(plugin);
    }
}

Plasma::Containment *ShellCorona::addPanel(const QString &plugin)
{
    Plasma::Containment *panel = createContainment(plugin);
    if (!panel) {
        return 0;
    }

    QList<Plasma::Types::Location> availableLocations;
    availableLocations << Plasma::Types::LeftEdge << Plasma::Types::TopEdge << Plasma::Types::RightEdge << Plasma::Types::BottomEdge;

    foreach (const Plasma::Containment *cont, d->panelViews.keys()) {
        availableLocations.removeAll(cont->location());
    }

    Plasma::Types::Location loc;
    if (availableLocations.isEmpty()) {
        loc = Plasma::Types::TopEdge;
    } else {
        loc = availableLocations.first();
    }

    panel->setLocation(loc);
    switch (loc) {
    case Plasma::Types::LeftEdge:
    case Plasma::Types::RightEdge:
        panel->setFormFactor(Plasma::Types::Vertical);
        break;
    default:
        panel->setFormFactor(Plasma::Types::Horizontal);
        break;
    }

    Q_ASSERT(panel);
    d->waitingPanels << panel;
    createWaitingPanels();

    const QPoint cursorPos(QCursor::pos());
    foreach (QScreen *screen, QGuiApplication::screens()) {
        //d->panelViews.contains(panel) == false iff addPanel is executed in a startup script
        if (screen->geometry().contains(cursorPos) && d->panelViews.contains(panel)) {
            d->panelViews[panel]->setScreen(screen);
            break;
        }
    }
    return panel;
}

int ShellCorona::screenForContainment(const Plasma::Containment *containment) const
{
    //case in which this containment is child of an applet, hello systray :)
    if (Plasma::Applet *parentApplet = qobject_cast<Plasma::Applet *>(containment->parent())) {
        if (Plasma::Containment* cont = parentApplet->containment()) {
            return screenForContainment(cont);
        } else {
            return -1;
        }
    }

    for (int i = 0; i < d->views.count(); i++) {
        if (d->views[i]->containment() == containment) {
            return i;
        }
    }

    PanelView *view = d->panelViews.value(containment);
    if (view) {
        QScreen *screen = view->screen();
        for (int i = 0; i < d->views.count(); i++) {
            if (d->views[i]->screen() == screen) {
                return i;
            }
        }
    }
    return -1;
}

void ShellCorona::activityOpened()
{
    Activity *activity = qobject_cast<Activity *>(sender());
    if (activity) {
        QList<Plasma::Containment*> cs = importLayout(activity->config());
        for (Plasma::Containment *containment : cs) {
            insertContainment(activity->name(), containment->lastScreen(), containment);
        }
    }
}

void ShellCorona::activityClosed()
{
    Activity *activity = qobject_cast<Activity *>(sender());
    if (activity) {
        KConfigGroup cg = activity->config();
        exportLayout(cg, d->desktopContainments.value(activity->name()).values());
    }
}

void ShellCorona::activityRemoved()
{
    //when an activity is removed delete all associated desktop containments
    Activity *activity = qobject_cast<Activity *>(sender());
    if (activity) {
        QHash< int, Plasma::Containment* > containmentHash = d->desktopContainments.take(activity->name());
        for (auto a : containmentHash) {
            a->destroy();
        }
    }
}

void ShellCorona::insertContainment(const QString &activity, int screenNum, Plasma::Containment *containment)
{
    Plasma::Containment *cont = d->desktopContainments.value(activity).value(screenNum);
    if (containment == cont) {
        return;
    }

    Q_ASSERT(!d->desktopContainments[activity].values().contains(containment));

    if (cont) {
        //containment should always be valid, it's been known to get in a mess
        //so guard anyway
        Q_ASSERT(cont);
        if (cont) {
            disconnect(cont, SIGNAL(destroyed(QObject*)),
                   this, SLOT(desktopContainmentDestroyed(QObject*)));
            cont->destroy();
        }
    }
    d->desktopContainments[activity][screenNum] = containment;

    //when a containment gets deleted update our map of containments
    connect(containment, SIGNAL(destroyed(QObject*)), this, SLOT(desktopContainmentDestroyed(QObject*)));
}

void ShellCorona::desktopContainmentDestroyed(QObject *obj)
{
    // when QObject::destroyed arrives, ~Plasma::Containment has run,
    // members of Containment are not accessible anymore,
    // so keep ugly bookeeping by hand
    auto containment = static_cast<Plasma::Containment*>(obj);
    for (auto a : d->desktopContainments) {
        QMutableHashIterator<int, Plasma::Containment *> it(a);
        while (it.hasNext()) {
            it.next();
            if (it.value() == containment) {
                it.remove();
                return;
            }
        }
    }
}

KScreen::Config* ShellCorona::screensConfiguration() const
{
    return d->screenConfiguration;
}

void ShellCorona::showOpenGLNotCompatibleWarning()
{
    static bool s_multipleInvokations = false;
    if (s_multipleInvokations) {
        return;
    }
    s_multipleInvokations = true;
    KMessageBox::error(nullptr,
                       i18n("Your graphics hardware does not support OpenGL (ES) 2. Plasma will abort now."),
                       i18n("Incompatible OpenGL version detected")
                      );
    // this doesn't work and I have no idea why.
    QCoreApplication::exit(1);
}

// Desktop corona handler


#include "moc_shellcorona.cpp"

