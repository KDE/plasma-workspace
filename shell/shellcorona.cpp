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

#include <KScreen/Config>
#include <kscreen/configmonitor.h>

#include "config-ktexteditor.h" // HAVE_KTEXTEDITOR


#include "activity.h"
#include "desktopview.h"
#include "panelview.h"
#include "scripting/desktopscriptengine.h"
#include "widgetexplorer/widgetexplorer.h"
#include "configview.h"
#include "shellpluginloader.h"
#include "shellmanager.h"
#include "osd.h"
#if HAVE_KTEXTEDITOR
#include "interactiveconsole.h"
#endif

#include "plasmashelladaptor.h"

static const int s_configSyncDelay = 10000; // 10 seconds

class ShellCorona::Private {
public:
    Private(ShellCorona *corona)
        : q(corona),
          activityController(new KActivities::Controller(q)),
          activityConsumer(new KActivities::Consumer(q)),
          addPanelAction(nullptr),
          addPanelsMenu(nullptr),
          screenConfiguration(nullptr)
    {
        appConfigSyncTimer.setSingleShot(true);
        appConfigSyncTimer.setInterval(s_configSyncDelay);
        connect(&appConfigSyncTimer, &QTimer::timeout, q, &ShellCorona::syncAppConfig);

        waitingPanelsTimer.setSingleShot(true);
        waitingPanelsTimer.setInterval(250);
        connect(&waitingPanelsTimer, &QTimer::timeout, q, &ShellCorona::createWaitingPanels);
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
#if HAVE_KTEXTEDITOR
    QWeakPointer<InteractiveConsole> console;
#endif

    KScreen::Config *screenConfiguration;
    QTimer waitingPanelsTimer;
    QTimer appConfigSyncTimer;
};

static QScreen *outputToScreen(KScreen::Output *output)
{
    if (!output)
        return 0;
    foreach(QScreen *screen, QGuiApplication::screens()) {
        if (screen->name() == output->name()) {
            return screen;
        }
    }
    return 0;
}

static KScreen::Output *screenToOutput(QScreen* screen, KScreen::Config* config)
{
    foreach(KScreen::Output* output, config->connectedOutputs()) {
        if (screen->name() == output->name()) {
            return output;
        }
    }
    return 0;
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

    const QString themeGroupKey = QStringLiteral("Theme");
    const QString themeNameKey = QStringLiteral("name");

    QString themeName;

    KConfigGroup plasmarc(KSharedConfig::openConfig("plasmarc"), themeGroupKey);
    themeName = plasmarc.readEntry(themeNameKey, themeName);

    if (themeName.isEmpty()) {
        const KConfigGroup lnfCfg = KConfigGroup(KSharedConfig::openConfig(
                                                lookAndFeelPackage().filePath("defaults")),
                                                themeGroupKey
                                           );
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
    return (a->isPrimary() && !b->isPrimary())
            || ((a->isPrimary() == b->isPrimary()) && (a->pos().x() < b->pos().x()) || (a->pos().x() == b->pos().x() && a->pos().y() < b->pos().y()));
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
    }

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
            insertContainment(containment->activity(), screen, containment);
        }
    }

    KScreen::ConfigMonitor::instance()->addConfig(d->screenConfiguration);
    for (KScreen::Output *output : sortOutputs(d->screenConfiguration->connectedOutputs())) {
        addOutput(output);
    }
    connect(d->screenConfiguration, &KScreen::Config::outputAdded, this, &ShellCorona::addOutput);
    connect(d->screenConfiguration, &KScreen::Config::primaryOutputChanged,
            this, &ShellCorona::primaryOutputChanged);

    if (!d->waitingPanels.isEmpty()) {
        d->waitingPanelsTimer.start();
    }
}

void ShellCorona::primaryOutputChanged()
{
    KScreen::Config *current = d->screenConfiguration;
    if (!current->primaryOutput())
        return;
    QScreen *newPrimary = outputToScreen(current->primaryOutput());
    int i=0;
    foreach(DesktopView *view, d->views) {
        if (view->screen() == newPrimary)
            break;
        i++;
    }
    QScreen *oldPrimary = d->views.first()->screen();
    qDebug() << "primary changed!" << oldPrimary->name() << newPrimary->name() << i;
//
//     //if it was not found, it means that addOutput hasn't been called yet
    if (i>=d->views.count() || i==0)
        return;
//
    Q_ASSERT(oldPrimary != newPrimary);
    Q_ASSERT(d->views[0]->screen() != d->views[i]->screen());
    Q_ASSERT(d->views[0]->screen() == oldPrimary);
    Q_ASSERT(d->views[0]->screen() != newPrimary);
    Q_ASSERT(d->views[0]->geometry() == oldPrimary->geometry());
    qDebug() << "adapting" << newPrimary->geometry() << d->views[0]->containment()->wallpaper()
                           << oldPrimary->geometry() << d->views[i]->containment()->wallpaper() << i;

    d->views[0]->setScreen(newPrimary);
    d->views[i]->setScreen(oldPrimary);
    screenInvariants();

    QList<Plasma::Containment*> panels;
    foreach(PanelView *panel, d->panelViews) {
        if (panel->screen() == oldPrimary)
            panel->setScreen(newPrimary);
        else if (panel->screen() == newPrimary)
            panel->setScreen(oldPrimary);
    }
}

void ShellCorona::screenInvariants() const
{
    Q_ASSERT(d->views.count() <= QGuiApplication::screens().count());
    Q_ASSERT(!d->screenConfiguration->primaryOutput() || !d->views.isEmpty() || outputToScreen(d->screenConfiguration->primaryOutput()) == d->views.first()->screen());
    QScreen *s = d->views.isEmpty() ? nullptr : d->views[0]->screen();
    if (!s) {
        qWarning() << "error: couldn't find primary output" << d->screenConfiguration->primaryOutput();
        return;
    }

    Q_ASSERT(d->views[0]->screen()->name() == s->name());
    if (!ShellManager::s_forceWindowed) {
        Q_ASSERT(d->views[0]->geometry() == s->geometry());
    }

    QSet<QScreen*> screens;
    foreach(DesktopView *view, d->views) {
        Q_ASSERT(!screens.contains(view->screen()));
        screens.insert(view->screen());
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
    DesktopView *view = 0;
    foreach (DesktopView *v, d->views) {
        if (v->containment() && v->containment()->screen() == id) {
            view = v;
            break;
        }
    }

    if (view) {
        return view->geometry();
    }

    //each screen should have a view
    qWarning() << "requesting unexisting screen" << id;
    QScreen *s = outputToScreen(d->screenConfiguration->primaryOutput());
    return s ? s->geometry() : QRect();
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    const QRect screenGeo(screenGeometry(id));

    DesktopView *view = 0;
    if (id >= 0 && id < d->views.count()) {
        view = d->views[id];
    }

    if (view) {
        QRegion r = view->geometry();
        QRect panelRect;
        foreach (PanelView *v, d->panelViews) {
            panelRect = v->geometry();
            if (v->containment()->screen() == id && v->visibilityMode() != PanelView::AutoHide) {
                //don't use panel positions, because sometimes the panel can be moved around
                switch (view->location()) {
                case Plasma::Types::TopEdge:
                    panelRect.moveTop(qMin(panelRect.top(), screenGeo.top()));
                    break;

                case Plasma::Types::BottomEdge:
                    panelRect.moveBottom(qMax(panelRect.bottom(), screenGeo.bottom()));
                    break;

                case Plasma::Types::LeftEdge:
                    panelRect.moveLeft(qMin(panelRect.left(), screenGeo.left()));
                    break;

                case Plasma::Types::RightEdge:
                    panelRect.moveRight(qMax(panelRect.right(), screenGeo.right()));
                    break;

                default:
                    break;
                }
                r -= panelRect;
            }
        }
        return r;
    } else {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = outputToScreen(d->screenConfiguration->primaryOutput());
        return s ? s->availableGeometry() : QRegion();
    }
}

QRect ShellCorona::availableScreenRect(int id) const
{
    if (id < 0) {
        id = 0;
    }

    const QRect screenGeo(screenGeometry(id));
    QRect r(screenGeometry(id));

    foreach (PanelView *view, d->panelViews) {
        if (view->containment()->screen() == id && view->visibilityMode() != PanelView::AutoHide) {
            QRect v = view->geometry();
            switch (view->location()) {
                case Plasma::Types::TopEdge:
                    //neutralize eventual panel positioning due to drag handle
                    v.moveTop(qMin(v.top(), screenGeo.top()));
                    if (v.bottom() > r.top()) {
                        r.setTop(v.bottom());
                    }
                    break;

                case Plasma::Types::BottomEdge:
                    v.moveBottom(qMax(v.bottom(), screenGeo.bottom()));
                    if (v.top() < r.bottom()) {
                        r.setBottom(v.top());
                    }
                    break;

                case Plasma::Types::LeftEdge:
                    v.moveLeft(qMin(v.left(), screenGeo.left()));
                    if (v.right() > r.left()) {
                        r.setLeft(v.right());
                    }
                    break;

                case Plasma::Types::RightEdge:
                    v.moveRight(qMax(v.right(), screenGeo.right()));
                    if (v.left() < r.right()) {
                        r.setRight(v.left());
                    }
                    break;

                default:
                    break;
            }
        }
    }

    return r;
}

PanelView *ShellCorona::panelView(Plasma::Containment *containment) const
{
    return d->panelViews.value(containment);
}

///// SLOTS

void ShellCorona::shiftViews(int idx, int delta, int until)
{
    for (int i = idx; i<until; ++i) {
        d->views[i]->setScreen(d->views[i-delta]->screen());
        d->views[i]->adaptToScreen();
    }
}

void ShellCorona::outputEnabledChanged()
{
    addOutput(qobject_cast<KScreen::Output*>(sender()));
}

void ShellCorona::addOutput(KScreen::Output *output)
{
    connect(output, &KScreen::Output::isEnabledChanged, this, &ShellCorona::outputEnabledChanged, Qt::UniqueConnection);
    if (!output->isEnabled())
        return;
    QScreen *screen = outputToScreen(output);
    Q_ASSERT(screen);

    //FIXME: QScreen doesn't have any idea of "this qscreen is clone of this other one
    //so this ultra inefficient heuristic has to stay until we have a slightly better api
    foreach (QScreen *s, QGuiApplication::screens()) {
        if (s->geometry().contains(screen->geometry(), false) &&
            s->geometry().width() > screen->geometry().width() &&
            s->geometry().height() > screen->geometry().height()) {
            return;
        }
    }

    int insertPosition = 0;
    foreach (DesktopView *view, d->views) {
        KScreen::Output *out = screenToOutput(view->screen(), d->screenConfiguration);
        if (outputLess(output, out))
            break;

        insertPosition++;
    }

    DesktopView *view = new DesktopView(this, screen);

    //We have to do it in a lambda,
    connect(screen, &QObject::destroyed, this, [=]() { removeDesktop(view); });

    d->views.insert(insertPosition, view);
    shiftViews(insertPosition+1, 1, d->views.count()-1);

    const QString currentActivity = d->activityController->currentActivity();
    Plasma::Containment *containment = d->desktopContainments[currentActivity].value(insertPosition);
    if (!containment) {
        containment = createContainmentForActivity(currentActivity, insertPosition);
    }
    if (!containment) {
        return;
    }

    QAction *removeAction = containment->actions()->action("remove");
    if (removeAction) {
        removeAction->deleteLater();
    }

    view->setContainment(containment);

   // connect(screen, SIGNAL(destroyed(QObject*)), SLOT(screenRemoved(QObject*)));
    view->show();

    //were there any panels for this screen before it popped up?
    if (!d->waitingPanels.isEmpty()) {
        d->waitingPanelsTimer.start();
    }

    emit availableScreenRectChanged();
    emit availableScreenRegionChanged();

    screenInvariants();
}

void ShellCorona::removeDesktop(DesktopView *view)
{
    int idx = d->views.indexOf(view);
    DesktopView *lastView = d->views.takeAt(d->views.count()-1);
    lastView->deleteLater();

    shiftViews(idx, -1, d->views.count()-1);

    screenInvariants();
}

void ShellCorona::removePanel(QObject *c)
{
    Plasma::Containment *cont = qobject_cast<Plasma::Containment*>(c);

    d->panelViews[cont]->deleteLater();
    d->waitingPanels << cont;
    d->panelViews.remove(cont);
}

Plasma::Containment *ShellCorona::createContainmentForActivity(const QString& activity, int screenNum)
{
    if (d->desktopContainments.contains(activity) &&
        d->desktopContainments[activity].contains(screenNum) &&
        d->desktopContainments.value(activity).value(screenNum)) {
        return d->desktopContainments[activity][screenNum];
    }

    QString plugin = "org.kde.desktopcontainment";
    if (d->activities.contains(activity)) {
      //  plugin = d->activities.value(activity)->defaultPlugin();
    }

    Plasma::Containment *containment = createContainment(d->desktopDefaultsConfig.readEntry("Containment", plugin));

    if (containment) {
        containment->setActivity(activity);
        insertContainment(activity, screenNum, containment);
    }

    return containment;
}

void ShellCorona::createWaitingPanels()
{
    QList<Plasma::Containment *> stillWaitingPanels;

    foreach (Plasma::Containment *cont, d->waitingPanels) {
        //ignore non existing (yet?) screens
        int requestedScreen = cont->lastScreen();
        if (requestedScreen < 0)
            ++requestedScreen;

        if (requestedScreen > (d->views.size() - 1)) {
            stillWaitingPanels << cont;
            continue;
        }

        d->panelViews[cont] = new PanelView(this);

        Q_ASSERT(qBound(0, requestedScreen, d->views.size() -1) == requestedScreen);
        QScreen *screen = d->views[requestedScreen]->screen();

        d->panelViews[cont]->setScreen(screen);
        d->panelViews[cont]->setContainment(cont);

        connect(cont, SIGNAL(destroyed(QObject*)), this, SLOT(containmentDeleted(QObject*)));
        connect(screen, SIGNAL(destroyed(QObject*)), this, SLOT(removePanel(QObject*)));
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
}

void ShellCorona::toggleWidgetExplorer()
{
    const QPoint cursorPos = QCursor::pos();
    foreach (DesktopView *view, d->views) {
        if (view->screen()->geometry().contains(cursorPos)) {
            //The view QML has to provide something to display the activity explorer
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
    qDebug() << "TODO: Toggling dashboard view";

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
        return;
    }

#if HAVE_KTEXTEDITOR
    InteractiveConsole *console = d->console.data();
    if (!console) {
        d->console = console = new InteractiveConsole(this);
    }
    d->console.data()->setMode(InteractiveConsole::PlasmaConsole);

    KWindowSystem::setOnDesktop(console->winId(), KWindowSystem::currentDesktop());
    console->show();
    console->raise();
    KWindowSystem::forceActiveWindow(console->winId());
#endif
}

void ShellCorona::loadScriptInInteractiveConsole(const QString &script)
{
#if HAVE_KTEXTEDITOR
    showInteractiveConsole();
    if (d->console) {
        d->console.data()->loadScript(script);
    }
#endif
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
    qDebug() << "Activity changed:" << newActivity;

    for (int i = 0; i < d->views.count(); ++i) {
        Plasma::Containment *c = d->desktopContainments[newActivity].value(i);
        if (!c) {
            c = createContainmentForActivity(newActivity, i);
        }
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
        qDebug() << "you're late." << id;
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
    Plasma::Containment *c = createContainmentForActivity(id, -1);
    c->config().writeEntry("lastScreen", 0);
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
        scriptEngine.evaluateScript(plugin.right(plugin.length() - qstrlen("plasma-desktop-template:")));
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

    for (int i = 0; i < d->views.size(); i++) {
        if (d->views[i]->containment() == containment) {
            return i;
        }
    }

    PanelView *view = d->panelViews.value(containment);
    if (view) {
        QScreen *screen = view->screen();
        for (int i = 0; i < d->views.size(); i++) {
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
    if (d->desktopContainments.contains(activity) &&
        d->desktopContainments[activity].contains(screenNum)) {
        Plasma::Containment *containment = d->desktopContainments[activity][screenNum];

        //containment should always be valid, it's been known to get in a mess
        //so guard anyway
        Q_ASSERT(containment);
        if (containment) {
            disconnect(containment, SIGNAL(destroyed(QObject*)),
                   this, SLOT(desktopContainmentDestroyed(QObject*)));
            containment->destroy();
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

Plasma::Package ShellCorona::lookAndFeelPackage() const
{
    if (!d->lookNFeelPackage.isValid()) {
        d->lookNFeelPackage = ShellPluginLoader::self()->loadPackage("Plasma/LookAndFeel");
        //TODO: make loading from config once we have some UI for setting the package
        d->lookNFeelPackage.setPath("org.kde.lookandfeel");
    }

    return d->lookNFeelPackage;
}

KScreen::Config* ShellCorona::screensConfiguration() const
{
    return d->screenConfiguration;
}

// Desktop corona handler


#include "moc_shellcorona.cpp"

