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

#include <config-plasma.h>

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QQmlContext>
#include <QDBusConnection>

#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <Plasma/Package>
#include <Plasma/PluginLoader>
#include <kactivities/controller.h>
#include <kactivities/consumer.h>
#include <ksycoca.h>
#include <KGlobalAccel>
#include <KAuthorized>
#include <KWindowSystem>
#include <kdeclarative/kdeclarative.h>
#include <kdeclarative/qmlobject.h>
#include <KMessageBox>

#include <KScreen/Config>
#include <KScreen/ConfigMonitor>
#include <KScreen/GetConfigOperation>
#include <KScreen/Output>

#include <KPackage/PackageLoader>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

#include "config-ktexteditor.h" // HAVE_KTEXTEDITOR

#include "alternativeshelper.h"
#include "activity.h"
#include "desktopview.h"
#include "panelview.h"
#include "scripting/scriptengine.h"
#include "plasmaquick/configview.h"
#include "shellmanager.h"
#include "osd.h"
#include "waylanddialogfilter.h"

#include "plasmashelladaptor.h"

#ifndef NDEBUG
    #define CHECK_SCREEN_INVARIANTS screenInvariants();
#else
    #define CHECK_SCREEN_INVARIANTS
#endif

#if HAVE_X11
#include <NETWM>
#include <QtX11Extras/QX11Info>
#endif


static const int s_configSyncDelay = 10000; // 10 seconds

ShellCorona::ShellCorona(QObject *parent)
    : Plasma::Corona(parent),
      m_activityController(new KActivities::Controller(this)),
      m_activityConsumer(new KActivities::Consumer(this)),
      m_addPanelAction(nullptr),
      m_addPanelsMenu(nullptr),
      m_interactiveConsole(nullptr),
      m_screenConfiguration(nullptr),
      m_waylandPlasmaShell(nullptr)
{
    setupWaylandIntegration();
    qmlRegisterUncreatableType<DesktopView>("org.kde.plasma.shell", 2, 0, "Desktop", QStringLiteral("It is not possible to create objects of type Desktop"));
    qmlRegisterUncreatableType<PanelView>("org.kde.plasma.shell", 2, 0, "Panel", QStringLiteral("It is not possible to create objects of type Panel"));

    m_lookAndFeelPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    if (!packageName.isEmpty()) {
        m_lookAndFeelPackage.setPath(packageName);
    }

    connect(this, &Plasma::Corona::containmentCreated, this, [this] (Plasma::Containment *c) {
        executeSetupPlasmoidScript(c, c);
    });

    connect(this, &Plasma::Corona::availableScreenRectChanged, this, &Plasma::Corona::availableScreenRegionChanged);

    m_appConfigSyncTimer.setSingleShot(true);
    m_appConfigSyncTimer.setInterval(s_configSyncDelay);
    connect(&m_appConfigSyncTimer, &QTimer::timeout, this, &ShellCorona::syncAppConfig);

    m_waitingPanelsTimer.setSingleShot(true);
    m_waitingPanelsTimer.setInterval(250);
    connect(&m_waitingPanelsTimer, &QTimer::timeout, this, &ShellCorona::createWaitingPanels);

    m_reconsiderOutputsTimer.setSingleShot(true);
    m_reconsiderOutputsTimer.setInterval(1000);
    connect(&m_reconsiderOutputsTimer, &QTimer::timeout, this, &ShellCorona::reconsiderOutputs);

    m_desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package().filePath("defaults")), "Desktop");

    new PlasmaShellAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/PlasmaShell"), this);

    connect(this, &Plasma::Corona::startupCompleted, this,
            []() {
                qDebug() << "Plasma Shell startup completed";
                QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                               QStringLiteral("/KSplash"),
                                               QStringLiteral("org.kde.KSplash"),
                                               QStringLiteral("setStage"));
                ksplashProgressMessage.setArguments(QList<QVariant>() << QStringLiteral("desktop"));
                QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
            });

    // Look for theme config in plasmarc, if it isn't configured, take the theme from the
    // LookAndFeel package, if either is set, change the default theme

    connect(qApp, &QCoreApplication::aboutToQuit, [this]() {
        //saveLayout is a slot but arguments not compatible
        saveLayout();
    });

    connect(this, &ShellCorona::containmentAdded,
            this, &ShellCorona::handleContainmentAdded);

    QAction *dashboardAction = actions()->addAction(QStringLiteral("show dashboard"));
    QObject::connect(dashboardAction, &QAction::triggered,
                     this, &ShellCorona::setDashboardShown);
    dashboardAction->setText(i18n("Show Desktop"));
    connect(KWindowSystem::self(), &KWindowSystem::showingDesktopChanged, [dashboardAction](bool showing) {
        dashboardAction->setText(showing ? i18n("Hide Desktop") : i18n("Show Desktop"));
        dashboardAction->setChecked(showing);
    });

    dashboardAction->setAutoRepeat(true);
    dashboardAction->setCheckable(true);
    dashboardAction->setIcon(QIcon::fromTheme(QStringLiteral("dashboard-show")));
    dashboardAction->setData(Plasma::Types::ControlAction);
    KGlobalAccel::self()->setGlobalShortcut(dashboardAction, Qt::CTRL + Qt::Key_F12);

    checkAddPanelAction();
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), this, SLOT(checkAddPanelAction(QStringList)));


    //Activity stuff
    QAction *activityAction = actions()->addAction(QStringLiteral("manage activities"));
    connect(activityAction, &QAction::triggered,
            this, &ShellCorona::toggleActivityManager);
    activityAction->setText(i18n("Activities..."));
    activityAction->setIcon(QIcon::fromTheme(QStringLiteral("preferences-activities")));
    activityAction->setData(Plasma::Types::ConfigureAction);
    activityAction->setShortcut(QKeySequence(QStringLiteral("alt+d, alt+a")));
    activityAction->setShortcutContext(Qt::ApplicationShortcut);

    KGlobalAccel::self()->setGlobalShortcut(activityAction, Qt::META + Qt::Key_Q);

    QAction *stopActivityAction = actions()->addAction(QStringLiteral("stop current activity"));
    QObject::connect(stopActivityAction, &QAction::triggered,
                     this, &ShellCorona::stopCurrentActivity);

    stopActivityAction->setText(i18n("Stop Current Activity"));
    stopActivityAction->setData(Plasma::Types::ControlAction);
    stopActivityAction->setVisible(false);


    KGlobalAccel::self()->setGlobalShortcut(stopActivityAction, Qt::META + Qt::Key_S);

    connect(m_activityConsumer, &KActivities::Consumer::currentActivityChanged, this, &ShellCorona::currentActivityChanged);
    connect(m_activityConsumer, &KActivities::Consumer::activityAdded, this, &ShellCorona::activityAdded);
    connect(m_activityConsumer, SIGNAL(activityRemoved(QString)), this, SLOT(activityRemoved(QString)));

    new Osd(this);

    qApp->installEventFilter(this);
}

ShellCorona::~ShellCorona()
{
    qDeleteAll(m_views);
    m_views.clear();
    qDeleteAll(containments());
    qDeleteAll(m_panelViews);
    m_panelViews.clear();
}

bool ShellCorona::eventFilter(QObject *watched, QEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    if (event->type() == QEvent::PlatformSurface &&
        watched->inherits("PlasmaQuick::Dialog")) {
        QPlatformSurfaceEvent *se = static_cast<QPlatformSurfaceEvent *>(event);
        if (se->surfaceEventType() == QPlatformSurfaceEvent::SurfaceCreated) {
            if (QGuiApplication::platformName().startsWith(
                QLatin1String("wayland"), Qt::CaseInsensitive)) {
                WaylandDialogFilter::install(qobject_cast<QWindow *>(watched), this);
            }
        }
    }
#endif

    return QObject::eventFilter(watched, event);
}

KPackage::Package ShellCorona::lookAndFeelPackage()
{
    return m_lookAndFeelPackage;
}

void ShellCorona::setShell(const QString &shell)
{
    if (m_shell == shell) {
        return;
    }

    m_shell = shell;
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Shell"));
    package.setPath(shell);
    package.setAllowExternalPaths(true);
    setKPackage(package);
    m_desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(package.filePath("defaults")), "Desktop");

        const QString themeGroupKey = QStringLiteral("Theme");
    const QString themeNameKey = QStringLiteral("name");

    QString themeName;

    KConfigGroup plasmarc(KSharedConfig::openConfig(QStringLiteral("plasmarc")), themeGroupKey);
    themeName = plasmarc.readEntry(themeNameKey, themeName);

    if (themeName.isEmpty()) {
        KConfigGroup shellCfg = KConfigGroup(KSharedConfig::openConfig(package.filePath("defaults")), "Theme");

        themeName = shellCfg.readEntry("name", "default");
        KConfigGroup lnfCfg = KConfigGroup(KSharedConfig::openConfig(
                                                m_lookAndFeelPackage.filePath("defaults")),
                                                "plasmarc"
                                           );
        lnfCfg = KConfigGroup(&lnfCfg, themeGroupKey);
        themeName = lnfCfg.readEntry(themeNameKey, themeName);
    }

    if (!themeName.isEmpty()) {
        Plasma::Theme *t = new Plasma::Theme(this);
        t->setThemeName(themeName);
    }

    //FIXME: this would change the runtime platform to a fixed one if available
    // but a different way to load platform specific components is needed beforehand
    // because if we import and use two different components plugin, the second time
    // the import is called it will fail
   /* KConfigGroup cg(KSharedConfig::openConfig(package.filePath("defaults")), "General");
    KDeclarative::KDeclarative::setRuntimePlatform(cg.readEntry("DefaultRuntimePlatform", QStringList()));*/

    unload();


    /*
     * we want to make an initial load once we have the initial screen config and we have loaded the activities _IF_ KAMD is running
     * it is valid for KAMD to not be running.
     *
     * Potentially 2 async jobs
     *
     * here we connect for status changes from KAMD, and fetch the first config from kscreen.
     * load() will check that we have a kscreen config, and m_activityConsumer->serviceStatus() is not loading (i.e not unknown)
     *
     * It might seem that we only need this connection if the activityConsumer is currently in state Unknown, however
     * there is an issue where m_activityConsumer will start the kactivitymanagerd, as KAMD is starting the serviceStatus will be "not running"
     * Whilst we are loading the kscreen config, the event loop runs and we might find KAMD has started.
     * m_activityConsumer will change from "not running" to unknown, and might still be unknown when the kscreen fetching is complete.
     *
     * if that happens we want to continue monitoring for state changes, and only finally load when it is up.
     *
     * See https://bugs.kde.org/show_bug.cgi?id=342431 be careful about changing
     *
     * The unique connection makes sure we don't reload plasma if KAMD ever crashes and reloads, the signal is disconnected in the body of load
     */

    connect(m_activityConsumer, &KActivities::Consumer::serviceStatusChanged, this, &ShellCorona::load, Qt::UniqueConnection);

    connect(new KScreen::GetConfigOperation(KScreen::GetConfigOperation::NoEDID), &KScreen::GetConfigOperation::finished,
                this, [this](KScreen::ConfigOperation *op) {
                    if (op->hasError()) {
                        qWarning() << "Error found while setting up ShellCorona's KScreen: " << op->errorString();
                    }
                    m_screenConfiguration = qobject_cast<KScreen::GetConfigOperation*>(op)->config();
                    load();
                });
}

QString ShellCorona::shell() const
{
    return m_shell;
}

bool outputLess(const KScreen::OutputPtr &a, const KScreen::OutputPtr &b)
{
    return ((a->isEnabled() && !b->isEnabled())
         || (a->isEnabled() == b->isEnabled() && (a->isPrimary() && !b->isPrimary()))
         || (a->isPrimary() == b->isPrimary() && (a->pos().x() < b->pos().x()
         || (a->pos().x() == b->pos().x() && a->pos().y() < b->pos().y()))));
}

static QList<KScreen::OutputPtr> sortOutputs(const KScreen::OutputList &outputs)
{
    QList<KScreen::OutputPtr> ret = outputs.values();
    std::sort(ret.begin(), ret.end(), outputLess);
    return ret;
}

void ShellCorona::load()
{
    if (m_shell.isEmpty() ||
        m_activityConsumer->serviceStatus() == KActivities::Consumer::Unknown ||
        !m_screenConfiguration) {
        return;
    }

    disconnect(m_activityConsumer, &KActivities::Consumer::serviceStatusChanged, this, &ShellCorona::load);

    loadLayout("plasma-" + m_shell + "-appletsrc");

    checkActivities();
    if (containments().isEmpty()) {
        loadDefaultLayout();
        processUpdateScripts();
    } else {
        processUpdateScripts();
        foreach(Plasma::Containment *containment, containments()) {
            if (containment->formFactor() == Plasma::Types::Horizontal ||
                containment->formFactor() == Plasma::Types::Vertical) {
                //Don't give a view to containments that don't want one (negative lastscreen)
                //this is pretty mucha special case for the systray
                if (!m_waitingPanels.contains(containment) && containment->lastScreen() >= 0) {
                    m_waitingPanels << containment;
                }
            } else {
                //FIXME ideally fix this, or at least document the crap out of it
                int screen = containment->lastScreen();
                if (screen < 0) {
                    screen = m_desktopContainments[containment->activity()].count();
                    qDebug() << "last screen is < 0 so putting containment on screen " << screen;
                }
                insertContainment(containment->activity(), screen, containment);
            }
        }
    }

    KScreen::ConfigMonitor::instance()->addConfig(m_screenConfiguration);
    //we're not going through the connectedOutputs because we need to connect to all outputs
    for (const KScreen::OutputPtr &output : sortOutputs(m_screenConfiguration->outputs())) {
        addOutput(output);
    }
    connect(m_screenConfiguration.data(), &KScreen::Config::outputAdded, this, &ShellCorona::addOutput);
    connect(m_screenConfiguration.data(), &KScreen::Config::primaryOutputChanged, this, &ShellCorona::primaryOutputChanged);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ShellCorona::screenRemoved);

    if (!m_waitingPanels.isEmpty()) {
        m_waitingPanelsTimer.start();
    }

    if (config()->isImmutable() ||
        !KAuthorized::authorize(QStringLiteral("plasma/plasmashell/unlockedDesktop"))) {
        setImmutability(Plasma::Types::SystemImmutable);
    } else {
        KConfigGroup coronaConfig(config(), "General");
        setImmutability((Plasma::Types::ImmutabilityType)coronaConfig.readEntry("immutability", (int)Plasma::Types::Mutable));
    }
}

void ShellCorona::primaryOutputChanged()
{
    if (m_views.isEmpty()) {
        return;
    }

    const KScreen::OutputPtr output = m_screenConfiguration->primaryOutput();
    if (!output) {
        return;
    }

    QScreen *oldPrimary = m_views[0]->screen();
    QScreen *newPrimary = outputToScreen(output);
    if (!newPrimary || newPrimary == oldPrimary) {
        return;
    }

    bool screenAlreadyUsed = false;
    foreach(DesktopView *view, m_views){
        if(view->screen() == newPrimary)
            screenAlreadyUsed = true;
    }

    if(!screenAlreadyUsed){
        //This happens when a new primary output gets connected and primaryOutputChanged() is called before addOutput()
        //addOutput will take care of setting the primary screen
        return;
    }

    qDebug() << "primary changed!" << oldPrimary->name() << newPrimary->name();

    foreach (DesktopView *view, m_views) {
        if (view->screen() == newPrimary) {
            Q_ASSERT(m_views[0]->screen() != view->screen());

            Q_ASSERT(oldPrimary != newPrimary);
            Q_ASSERT(m_views[0]->screen() == oldPrimary);
            Q_ASSERT(m_views[0]->screen() != newPrimary);
            Q_ASSERT(m_views[0]->geometry() == oldPrimary->geometry());
            qDebug() << "adapting" << newPrimary->geometry() << oldPrimary->geometry();

            view->setScreen(oldPrimary);
            break;
        }
    }

    m_views[0]->setScreen(newPrimary);

    foreach (PanelView *panel, m_panelViews) {
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
    Q_ASSERT(m_views.count() <= QGuiApplication::screens().count());
    QScreen *s = m_views.isEmpty() ? nullptr : m_views[0]->screen();
    const KScreen::OutputPtr primaryOutput = m_screenConfiguration->primaryOutput();
    if (!s) {
        qWarning() << "error: couldn't find primary output" << primaryOutput;
        return;
    }

    QScreen* ks = outputToScreen(primaryOutput);
    Q_ASSERT(!ks || ks == s || !primaryOutput->isEnabled() || m_redundantOutputs.contains(primaryOutput));

    QSet<QScreen*> screens;
    int i = 0;
    foreach (const DesktopView *view, m_views) {
        QScreen *screen = view->screen();
        Q_ASSERT(!screens.contains(screen));
        Q_ASSERT(!m_redundantOutputs.contains(screenToOutput(screen)));
//         commented out because a different part of the code-base is responsible for this
//         and sometimes is not yet called here.
//         Q_ASSERT(!view->fillScreen() || view->geometry() == screen->geometry());
        Q_ASSERT(view->containment());
        Q_ASSERT(view->containment()->screen() == i || view->containment()->screen() == -1);
        Q_ASSERT(view->containment()->lastScreen() == i || view->containment()->lastScreen() == -1);
        Q_ASSERT(view->isVisible());

        foreach (const PanelView *panel, panelsForScreen(screen)) {
            Q_ASSERT(panel->containment());
            Q_ASSERT(panel->containment()->screen() == i || panel->containment()->screen() == -1);
            Q_ASSERT(panel->isVisible());
        }

        screens.insert(screen);
        ++i;
    }

    foreach (const KScreen::OutputPtr &out, m_redundantOutputs) {
        Q_ASSERT(isOutputRedundant(out));
    }

    if (m_views.isEmpty()) {
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
    qmlObj->rootContext()->setContextProperty(QStringLiteral("alternativesHelper"), helper);

    m_alternativesObjects << qmlObj;
    qmlObj->completeInitialization();
    connect(qmlObj->rootObject(), SIGNAL(visibleChanged(bool)),
            this, SLOT(alternativesVisibilityChanged(bool)));

    connect(applet, &Plasma::Applet::destroyedChanged, this, [this, qmlObj] (bool destroyed) {
        if (!destroyed) {
            return;
        }
        QMutableListIterator<KDeclarative::QmlObject *> it(m_alternativesObjects);
        while (it.hasNext()) {
            KDeclarative::QmlObject *obj = it.next();
            if (obj == qmlObj) {
                it.remove();
                obj->deleteLater();
            }
        }
    });
}

void ShellCorona::alternativesVisibilityChanged(bool visible)
{
    if (visible) {
        return;
    }

    QObject *root = sender();

    QMutableListIterator<KDeclarative::QmlObject *> it(m_alternativesObjects);
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
    if (m_shell.isEmpty()) {
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
    m_appConfigSyncTimer.start();
}

void ShellCorona::loadDefaultLayout()
{
    QString script = m_lookAndFeelPackage.filePath("layouts", QString(shell() + "-layout.js").toLatin1());

    if (script.isEmpty()) {
        script = package().filePath("defaultlayout");
    }

    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;

        WorkspaceScripting::ScriptEngine scriptEngine(this);

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
    WorkspaceScripting::ScriptEngine scriptEngine(this);

    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this,
            [](const QString &msg) {
                qWarning() << msg;
            });
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this,
            [](const QString &msg) {
                qDebug() << msg;
            });
    foreach (const QString &script, WorkspaceScripting::ScriptEngine::pendingUpdateScripts(this)) {
        QFile file(script);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
            QString code = file.readAll();
            scriptEngine.evaluateScript(code);
        } else {
            qWarning() << "Unable to open the script file" << script << "for reading";
        }
    }
}

KActivities::Controller *ShellCorona::activityController()
{
    return m_activityController;
}

int ShellCorona::numScreens() const
{
    //don't start loading screens until kscreen has finished initialising.
    if (!m_screenConfiguration) {
        return 0;
    }
    return m_screenConfiguration->outputs().count();
}

QRect ShellCorona::screenGeometry(int id) const
{
    if (id >= m_views.count() || id < 0) {
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = outputToScreen(m_screenConfiguration->primaryOutput());
        return s ? s->geometry() : QRect();
    }
    return m_views[id]->geometry();
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    if (id >= m_views.count() || id < 0) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = outputToScreen(m_screenConfiguration->primaryOutput());
        return s ? s->availableGeometry() : QRegion();
    }
    DesktopView *view = m_views[id];
    const QRect screenGeo(view->geometry());

    QRegion r = view->geometry();
    foreach (const PanelView *v, m_panelViews) {
        if (v->isVisible() && view->screen() == v->screen() && v->visibilityMode() != PanelView::AutoHide) {
            //if the panel is being moved around, we still want to calculate it from the edge
            r -= v->geometryByDistance(0);
        }
    }
    return r;
}

QRect ShellCorona::availableScreenRect(int id) const
{
    if (id >= m_views.count() || id < 0) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = outputToScreen(m_screenConfiguration->primaryOutput());
        return s ? s->availableGeometry() : QRect();
    }

    DesktopView *view = m_views[id];
    const QRect screenGeo(view->geometry());

    QRect r = view->geometry();
    foreach (PanelView *v, m_panelViews) {
        if (v->isVisible() && v->screen() == view->screen() && v->visibilityMode() != PanelView::AutoHide) {
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
    DesktopView *v = m_views.value(screenId);
    return v ? v->screen() : nullptr;
}

void ShellCorona::remove(DesktopView *desktopView)
{
    removeView(m_views.indexOf(desktopView));
}

PanelView *ShellCorona::panelView(Plasma::Containment *containment) const
{
    return m_panelViews.value(containment);
}

///// SLOTS

QList<PanelView *> ShellCorona::panelsForScreen(QScreen *screen) const
{
    QList<PanelView *> ret;
    foreach (PanelView *v, m_panelViews) {
        if (v->screen() == screen) {
            ret += v;
        }
    }
    return ret;
}

DesktopView* ShellCorona::desktopForScreen(QScreen* screen) const
{
    foreach (DesktopView *v, m_views) {
        if (v->screen() == screen) {
            return v;
        }
    }
    return Q_NULLPTR;
}

void ShellCorona::removeView(int idx)
{
    if (idx < 0 || idx >= m_views.count() || m_views.isEmpty()) {
        return;
    }

    bool panelsAltered = false;

    const QScreen *lastScreen = m_views.last()->screen();
    QMutableHashIterator<const Plasma::Containment *, PanelView *> it(m_panelViews);
    while (it.hasNext()) {
        it.next();
        PanelView *panelView = it.value();

        if (panelView->screen() == lastScreen) {
            m_waitingPanels << panelView->containment();
            it.remove();
            delete panelView;
            panelsAltered = true;
        }
    }

    for (int i = m_views.count() - 2; i >= idx; --i) {
        QScreen *screen = m_views[i + 1]->screen();
        QScreen *oldScreen = m_views[i]->screen();

        const bool wasVisible = m_views[idx]->isVisible();
        m_views[i]->setScreen(screen);

        if (wasVisible) {
            m_views[idx]->show(); //when destroying the screen, QScreen might have hidden the window
        }

        const QList<PanelView *> panels = panelsForScreen(oldScreen);
        panelsAltered = panelsAltered || !panels.isEmpty();
        foreach (PanelView *p, panels) {
            p->setScreen(screen);
        }
    }

    delete m_views.takeLast();

    if (panelsAltered) {
        emit availableScreenRectChanged();
    }
}

void ShellCorona::screenRemoved(QScreen* screen)
{
    if (DesktopView* v = desktopForScreen(screen))
        remove(v);
//     addOutput(screenToOutput(screen));
    m_reconsiderOutputsTimer.start();
}

void ShellCorona::outputEnabledChanged()
{
    addOutput(KScreen::OutputPtr(qobject_cast<KScreen::Output *>(sender()), [](void*){}));
}

bool ShellCorona::isOutputRedundant(const KScreen::OutputPtr &screen) const
{
    Q_ASSERT(screen->currentMode());
    const QRect geometry = screen->geometry();

    //FIXME: QScreen doesn't have any idea of "this qscreen is clone of this other one
    //so this ultra inefficient heuristic has to stay until we have a slightly better api
    foreach (const KScreen::OutputPtr &s, m_screenConfiguration->connectedOutputs()) {
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
    if (!m_screenConfiguration) {
        return;
    }
    foreach (const KScreen::OutputPtr &out, m_screenConfiguration->connectedOutputs()) {
        if (!out->isEnabled() || !out->currentMode()) {
//             qDebug() << "skip screen" << out << desktopForScreen(outputToScreen(out));
            continue;
        }

        if (m_redundantOutputs.contains(out)) {
            if (!isOutputRedundant(out)) {
//                 qDebug() << "not redundant anymore" << out;
                addOutput(out);
            }
        } else if (isOutputRedundant(out)) {
//             qDebug() << "new redundant screen" << out;

            if (DesktopView* v = desktopForScreen(outputToScreen(out)))
                remove(v);

            m_redundantOutputs.insert(out);
        }
//         else
//             qDebug() << "fine screen" << out;
    }

    CHECK_SCREEN_INVARIANTS
}

void ShellCorona::addOutput(const KScreen::OutputPtr &output)
{
    if (!output) {
        return;
    }

    connect(output.data(), &KScreen::Output::isEnabledChanged,
            this, &ShellCorona::outputEnabledChanged,
            Qt::UniqueConnection);
    connect(output.data(), &KScreen::Output::posChanged,
            &m_reconsiderOutputsTimer, static_cast<void (QTimer::*)()>(&QTimer::start),
            Qt::UniqueConnection);
    connect(output.data(), &KScreen::Output::currentModeIdChanged,
            &m_reconsiderOutputsTimer, static_cast<void (QTimer::*)()>(&QTimer::start),
            Qt::UniqueConnection);

    if (!output->isEnabled()) {
        m_redundantOutputs.remove(output);
        m_reconsiderOutputsTimer.start();
        return;
    }

    QScreen *screen = outputToScreen(output);
    Q_ASSERT(screen);

    if (isOutputRedundant(output)) {
        m_redundantOutputs.insert(output);
        return;
    } else {
        m_redundantOutputs.remove(output);
    }

    int insertPosition = 0;
    foreach (DesktopView *view, m_views) {
        const KScreen::OutputPtr out = screenToOutput(view->screen());
        if (outputLess(output, out)) {
            break;
        }

        insertPosition++;
    }

    QScreen* newScreen = insertScreen(screen, insertPosition);

    DesktopView *view = new DesktopView(this, newScreen);

    Plasma::Containment *containment = createContainmentForActivity(m_activityController->currentActivity(), m_views.count());
    Q_ASSERT(containment);

    QAction *removeAction = containment->actions()->action(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }

    m_views.append(view);
    view->setContainment(containment);
    view->show();
    Q_ASSERT(newScreen == view->screen());

    //need to specifically call the reactToScreenChange, since when the screen is shown it's not yet
    //in the list. We still don't want to have an invisible view added.
    containment->reactToScreenChange();

    //were there any panels for this screen before it popped up?
    if (!m_waitingPanels.isEmpty()) {
        m_waitingPanelsTimer.start();
    }

    emit availableScreenRectChanged();

    CHECK_SCREEN_INVARIANTS
}

QScreen *ShellCorona::outputToScreen(const KScreen::OutputPtr &output) const
{
    if (!output) {
        return nullptr;
    }

    foreach (QScreen *screen, QGuiApplication::screens()) {
        if (screen->name() == output->name()) {
            return screen;
        }
    }

    return nullptr;
}

KScreen::OutputPtr ShellCorona::screenToOutput(QScreen *screen) const
{
    foreach (const KScreen::OutputPtr &output, m_screenConfiguration->connectedOutputs()) {
        if (screen->name() == output->name()) {
            return output;
        }
    }

    return KScreen::OutputPtr();
}

QScreen* ShellCorona::insertScreen(QScreen *screen, int idx)
{
    if (idx == m_views.count()) {
        return screen;
    }

    DesktopView *v = m_views[idx];
    QScreen *oldScreen = v->screen();
    v->setScreen(screen);
    Q_ASSERT(v->screen() == screen);
    foreach (PanelView *panel, m_panelViews) {
        if (panel->screen() == oldScreen) {
            panel->setScreen(screen);
        }
    }

    return insertScreen(oldScreen, idx+1);
}

Plasma::Containment *ShellCorona::createContainmentForActivity(const QString& activity, int screenNum)
{
    QHash<int, Plasma::Containment *> act = m_desktopContainments.value(activity);
    QHash<int, Plasma::Containment *>::const_iterator it = act.constFind(screenNum);
    if (it != act.constEnd()) {
        return *it;
    }

    QString plugin;
    Activity* a = m_activities.value(activity);
    if (a && !a->defaultPlugin().isEmpty()) {
        plugin = a->defaultPlugin();
    } else {
        plugin = m_desktopDefaultsConfig.readEntry("Containment", "org.kde.desktopcontainment");
    }

    Plasma::Containment *containment = containmentForScreen(screenNum, plugin, QVariantList());
    Q_ASSERT(containment);

    if (containment) {
        containment->setActivity(activity);
        insertContainment(activity, screenNum, containment);
    }

    return containment;
}

void ShellCorona::createWaitingPanels()
{
    QList<Plasma::Containment *> stillWaitingPanels;

    foreach (Plasma::Containment *cont, m_waitingPanels) {
        //ignore non existing (yet?) screens
        int requestedScreen = cont->lastScreen();
        if (requestedScreen < 0) {
            ++requestedScreen;
        }

        if (requestedScreen > (m_views.count() - 1)) {
            stillWaitingPanels << cont;
            continue;
        }

        Q_ASSERT(qBound(0, requestedScreen, m_views.count() - 1) == requestedScreen);
        QScreen *screen = m_views[requestedScreen]->screen();
        PanelView* panel = new PanelView(this, screen);
        connect(panel, &QWindow::visibleChanged, this, &Plasma::Corona::availableScreenRectChanged);
        connect(panel, &PanelView::locationChanged, this, &Plasma::Corona::availableScreenRectChanged);
        connect(panel, &PanelView::visibilityModeChanged, this, &Plasma::Corona::availableScreenRectChanged);
        connect(panel, &PanelView::thicknessChanged, this, &Plasma::Corona::availableScreenRectChanged);


        m_panelViews[cont] = panel;
        panel->setContainment(cont);
        panel->show();
        cont->reactToScreenChange();

        connect(cont, &QObject::destroyed, this, &ShellCorona::panelContainmentDestroyed);
    }
    m_waitingPanels = stillWaitingPanels;
    emit availableScreenRectChanged();
}

void ShellCorona::panelContainmentDestroyed(QObject *cont)
{
    auto view = m_panelViews.take(static_cast<Plasma::Containment*>(cont));
    view->deleteLater();
    emit availableScreenRectChanged();
}

void ShellCorona::handleContainmentAdded(Plasma::Containment *c)
{
    connect(c, &Plasma::Containment::showAddWidgetsInterface,
            this, &ShellCorona::toggleWidgetExplorer);
    // Why queued? this is usually triggered after a context menu closes
    // due to its sync,modal nature it may eat some mouse event from the scene
    // waiting a bit to create a new window, the dialog seems to reliably
    // avoid the eating of one click in the panel after the context menu is gone
    connect(c, &Plasma::Containment::appletAlternativesRequested,
            this, &ShellCorona::showAlternativesForApplet, Qt::QueuedConnection);

    connect(c, &Plasma::Containment::appletCreated, this, [this, c] (Plasma::Applet *applet) {
        executeSetupPlasmoidScript(c, applet);
    });
}

void ShellCorona::executeSetupPlasmoidScript(Plasma::Containment *containment, Plasma::Applet *applet)
{
    if (!applet->pluginInfo().isValid() || !containment->pluginInfo().isValid()) {
        return;
    }

    const QString scriptFile = m_lookAndFeelPackage.filePath("plasmoidsetupscripts", applet->pluginInfo().pluginName() + ".js");

    if (scriptFile.isEmpty()) {
        return;
    }

    WorkspaceScripting::ScriptEngine scriptEngine(this);

    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this,
            [](const QString &msg) {
                qWarning() << msg;
            });
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this,
            [](const QString &msg) {
                qDebug() << msg;
            });

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

    scriptEngine.globalObject().setProperty(QStringLiteral("applet"), scriptEngine.wrap(applet));
    scriptEngine.globalObject().setProperty(QStringLiteral("containment"), scriptEngine.wrap(containment));
    scriptEngine.evaluateScript(script, scriptFile);
}

void ShellCorona::toggleWidgetExplorer()
{
    const QPoint cursorPos = QCursor::pos();
    foreach (DesktopView *view, m_views) {
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
    foreach (DesktopView *view, m_views) {
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
    KWindowSystem::setShowingDesktop(show);
}

void ShellCorona::toggleDashboard()
{
    setDashboardShown(!KWindowSystem::showingDesktop());
}

void ShellCorona::showInteractiveConsole()
{
    if (KSharedConfig::openConfig()->isImmutable() || !KAuthorized::authorize(QStringLiteral("plasma-desktop/scripting_console"))) {
        delete m_interactiveConsole;
        m_interactiveConsole = 0;
        return;
    }

    if (!m_interactiveConsole) {
        const QString consoleQML = package().filePath("interactiveconsole");
        if (consoleQML.isEmpty()) {
            return;
        }

        m_interactiveConsole = new KDeclarative::QmlObject(this);
        m_interactiveConsole->setInitializationDelayed(true);
        m_interactiveConsole->setSource(QUrl::fromLocalFile(consoleQML));

        QObject *engine = new WorkspaceScripting::ScriptEngine(this, m_interactiveConsole);
        m_interactiveConsole->rootContext()->setContextProperty(QStringLiteral("scriptEngine"), engine);

        m_interactiveConsole->completeInitialization();
        if (m_interactiveConsole->rootObject()) {
            connect(m_interactiveConsole->rootObject(), SIGNAL(visibleChanged(bool)),
                    this, SLOT(interactiveConsoleVisibilityChanged(bool)));
        }
    }

    if (m_interactiveConsole->rootObject()) {
        m_interactiveConsole->rootObject()->setProperty("mode", "desktop");
        m_interactiveConsole->rootObject()->setProperty("visible", true);
    }
}

void ShellCorona::loadScriptInInteractiveConsole(const QString &script)
{
    showInteractiveConsole();
    if (m_interactiveConsole) {
        m_interactiveConsole->rootObject()->setProperty("script", script);
    }
}

void ShellCorona::interactiveConsoleVisibilityChanged(bool visible)
{
    if (!visible) {
        m_interactiveConsole->deleteLater();
        m_interactiveConsole = nullptr;
    }
}

void ShellCorona::checkActivities()
{
    KActivities::Consumer::ServiceStatus status = m_activityController->serviceStatus();
    //qDebug() << "$%$%$#%$%$%Status:" << status;
    if (status != KActivities::Consumer::Running) {
        //panic and give up - better than causing a mess
        qDebug() << "ShellCorona::checkActivities is called whilst activity daemon is still connecting";
        return;
    }

    QStringList existingActivities = m_activityConsumer->activities();
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

    for (int i = 0; i < m_views.count(); ++i) {
        Plasma::Containment *c = createContainmentForActivity(newActivity, i);

        QAction *removeAction = c->actions()->action(QStringLiteral("remove"));
        if (removeAction) {
            removeAction->deleteLater();
        }
        m_views[i]->setContainment(c);
    }
}

void ShellCorona::activityAdded(const QString &id)
{
    //TODO more sanity checks
    if (m_activities.contains(id)) {
        qWarning() << "Activity added twice" << id;
        return;
    }

    Activity *a = new Activity(id, this);
    a->setDefaultPlugin(m_desktopDefaultsConfig.readEntry("Containment", "org.kde.desktopcontainment"));
    m_activities.insert(id, a);
}

void ShellCorona::activityRemoved(const QString &id)
{
    Activity *a = m_activities.take(id);
    a->deleteLater();
}

Activity *ShellCorona::activity(const QString &id)
{
    return m_activities.value(id);
}

void ShellCorona::insertActivity(const QString &id, Activity *activity)
{
    m_activities.insert(id, activity);
    Plasma::Containment *c = createContainmentForActivity(id, m_views.count());
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
    foreach (DesktopView *v, m_views) {
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
        if (group != QLatin1String("Applets")) {
            KConfigGroup subGroup(&oldCg, group);
            KConfigGroup newSubGroup(&newCg, group);
            subGroup.copyTo(&newSubGroup);

            KConfigGroup newSubGroup2(&newCg2, group);
            subGroup.copyTo(&newSubGroup2);
        }
    }

    newContainment->init();
    newCg.writeEntry("activityId", oldContainment->activity());
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
    QAction *removeAction = newContainment->actions()->action(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }
    view->setContainment(newContainment);
    newContainment->setActivity(oldContainment->activity());
    m_desktopContainments.remove(oldContainment->activity());
    insertContainment(oldContainment->activity(), screen, newContainment);

    //removing the focus from the item that is going to be destroyed
    //fixes a crash
    //delayout the destruction of the old containment fixes another crash
    view->rootObject()->setFocus(true, Qt::MouseFocusReason);
    QTimer::singleShot(2500, oldContainment, &Plasma::Applet::destroy);

    emit availableScreenRectChanged();

    return newContainment;
}

void ShellCorona::checkAddPanelAction(const QStringList &sycocaChanges)
{
    if (!sycocaChanges.isEmpty() && !sycocaChanges.contains(QStringLiteral("services"))) {
        return;
    }

    delete m_addPanelAction;
    m_addPanelAction = 0;

    delete m_addPanelsMenu;
    m_addPanelsMenu = 0;

    KPluginInfo::List panelContainmentPlugins = Plasma::PluginLoader::listContainmentsOfType(QStringLiteral("Panel"));

    auto filter = [](const KPluginMetaData &md) -> bool
    {
        return md.value(QStringLiteral("X-Plasma-Shell")) == qApp->applicationName() && md.value(QStringLiteral("X-Plasma-ContainmentCategories")).contains(QStringLiteral("panel"));
    };
    QList<KPluginMetaData> templates = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), filter);

    if (panelContainmentPlugins.count() + templates.count() == 1) {
        m_addPanelAction = new QAction(i18n("Add Panel"), this);
        m_addPanelAction->setData(Plasma::Types::AddAction);
        connect(m_addPanelAction, SIGNAL(triggered(bool)), this, SLOT(addPanel()));
    } else if (!panelContainmentPlugins.isEmpty()) {
        m_addPanelsMenu = new QMenu;
        m_addPanelAction = m_addPanelsMenu->menuAction();
        m_addPanelAction->setText(i18n("Add Panel"));
        m_addPanelAction->setData(Plasma::Types::AddAction);
        connect(m_addPanelsMenu, &QMenu::aboutToShow, this, &ShellCorona::populateAddPanelsMenu);
        connect(m_addPanelsMenu, SIGNAL(triggered(QAction*)), this, SLOT(addPanel(QAction*)));
    }

    if (m_addPanelAction) {
        m_addPanelAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        actions()->addAction(QStringLiteral("add panel"), m_addPanelAction);
    }
}

void ShellCorona::populateAddPanelsMenu()
{
    m_addPanelsMenu->clear();
    const KPluginInfo emptyInfo;

    KPluginInfo::List panelContainmentPlugins = Plasma::PluginLoader::listContainmentsOfType(QStringLiteral("Panel"));
    QMap<QString, QPair<KPluginInfo, KPluginMetaData> > sorted;
    foreach (const KPluginInfo &plugin, panelContainmentPlugins) {
        sorted.insert(plugin.name(), qMakePair(plugin, KPluginMetaData()));
    }

    auto filter = [](const KPluginMetaData &md) -> bool
    {
        return md.value(QStringLiteral("X-Plasma-Shell")) == qApp->applicationName() && md.value(QStringLiteral("X-Plasma-ContainmentCategories")).contains(QStringLiteral("panel"));
    };
    QList<KPluginMetaData> templates = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), filter);
    for (auto tpl : templates) {
        sorted.insert(tpl.name(), qMakePair(emptyInfo, tpl));
    }

    QMapIterator<QString, QPair<KPluginInfo, KPluginMetaData> > it(sorted);
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LayoutTemplate"));
    while (it.hasNext()) {
        it.next();
        QPair<KPluginInfo, KPluginMetaData> pair = it.value();
        if (pair.first.isValid()) {
            KPluginInfo plugin = pair.first;
            QAction *action = m_addPanelsMenu->addAction(i18n("Empty %1", plugin.name()));
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
                    QAction *action = m_addPanelsMenu->addAction(info.name());
                    action->setData(QStringLiteral("plasma-desktop-template:%1").arg(scriptFile));
                }
            }
        }
    }
}

void ShellCorona::addPanel()
{
    KPluginInfo::List panelPlugins = Plasma::PluginLoader::listContainmentsOfType(QStringLiteral("Panel"));

    if (!panelPlugins.isEmpty()) {
        addPanel(panelPlugins.first().pluginName());
    }
}

void ShellCorona::addPanel(QAction *action)
{
    const QString plugin = action->data().toString();
    if (plugin.startsWith(QLatin1String("plasma-desktop-template:"))) {
        WorkspaceScripting::ScriptEngine scriptEngine(this);

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

    foreach (const Plasma::Containment *cont, m_panelViews.keys()) {
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
    m_waitingPanels << panel;
    createWaitingPanels();

    const QPoint cursorPos(QCursor::pos());
    foreach (QScreen *screen, QGuiApplication::screens()) {
        //m_panelViews.contains(panel) == false iff addPanel is executed in a startup script
        if (screen->geometry().contains(cursorPos) && m_panelViews.contains(panel)) {
            m_panelViews[panel]->setScreen(screen);
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

    //if the desktop views already exist, base the decision upon them
    for (int i = 0; i < m_views.count(); i++) {
        if (m_views[i]->containment() == containment && containment->activity() == m_activityConsumer->currentActivity()) {
            return i;
        }
    }

    //if the panel views already exist, base upon them
    PanelView *view = m_panelViews.value(containment);
    if (view) {
        QScreen *screen = view->screen();
        for (int i = 0; i < m_views.count(); i++) {
            if (m_views[i]->screen() == screen) {
                return i;
            }
        }
    }

    //Failed? fallback on lastScreen()
    //lastScreen() is the correct screen for panels
    //It is also correct for desktops *that have the correct activity()*
    //a containment with lastScreen() == 0 but another activity,
    //won't be associated to a screen
//     qDebug() << "ShellCorona screenForContainment: " << containment << " Last screen is " << containment->lastScreen();
    for (int i = 0, count = m_screenConfiguration->outputs().count(); i<count; ++i) {
        if (containment->lastScreen() == i &&
            (containment->activity() == m_activityConsumer->currentActivity() ||
            containment->containmentType() == Plasma::Types::PanelContainment || containment->containmentType() == Plasma::Types::CustomPanelContainment)) {
            return i;
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
        exportLayout(cg, m_desktopContainments.value(activity->name()).values());
    }
}

void ShellCorona::activityRemoved()
{
    //when an activity is removed delete all associated desktop containments
    Activity *activity = qobject_cast<Activity *>(sender());
    if (activity) {
        QHash< int, Plasma::Containment* > containmentHash = m_desktopContainments.take(activity->name());
        for (auto a : containmentHash) {
            a->destroy();
        }
    }
}

void ShellCorona::nextActivity()
{
    const QStringList list = m_activityConsumer->activities(KActivities::Info::Running);
    if (list.isEmpty()) {
        return;
    }

    const int start = list.indexOf(m_activityController->currentActivity());
    const int i = (start + 1) % list.size();

    m_activityController->setCurrentActivity(list.at(i));
}

void ShellCorona::previousActivity()
{
    const QStringList list = m_activityConsumer->activities(KActivities::Info::Running);
    if (list.isEmpty()) {
        return;
    }

    const int start = list.indexOf(m_activityController->currentActivity());
    int i = start - 1;
    if(i < 0) {
        i = list.size() - 1;
    }

    m_activityController->setCurrentActivity(list.at(i));
}

void ShellCorona::stopCurrentActivity()
{
    const QStringList list = m_activityConsumer->activities(KActivities::Info::Running);
    if (list.isEmpty()) {
        return;
    }

    m_activityController->stopActivity(m_activityConsumer->currentActivity());
}

void ShellCorona::insertContainment(const QString &activity, int screenNum, Plasma::Containment *containment)
{
    Plasma::Containment *cont = m_desktopContainments.value(activity).value(screenNum);
    if (containment == cont) {
        return;
    }

    Q_ASSERT(!m_desktopContainments[activity].values().contains(containment));

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
    m_desktopContainments[activity][screenNum] = containment;

    //when a containment gets deleted update our map of containments
    connect(containment, SIGNAL(destroyed(QObject*)), this, SLOT(desktopContainmentDestroyed(QObject*)));
}

void ShellCorona::desktopContainmentDestroyed(QObject *obj)
{
    // when QObject::destroyed arrives, ~Plasma::Containment has run,
    // members of Containment are not accessible anymore,
    // so keep ugly bookeeping by hand
    auto containment = static_cast<Plasma::Containment*>(obj);
    for (auto a : m_desktopContainments) {
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

KScreen::ConfigPtr ShellCorona::screensConfiguration() const
{
    return m_screenConfiguration;
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

void ShellCorona::setupWaylandIntegration()
{
    if (!QGuiApplication::platformName().startsWith(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        return;
    }
    using namespace KWayland::Client;
    ConnectionThread *connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        return;
    }
    Registry *registry = new Registry(this);
    registry->create(connection);
    connect(registry, &Registry::plasmaShellAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_waylandPlasmaShell = registry->createPlasmaShell(name, version, this);
        }
    );
    registry->setup();
}

KWayland::Client::PlasmaShell *ShellCorona::waylandPlasmaShellInterface() const
{
    return m_waylandPlasmaShell;
}

// Desktop corona handler


#include "moc_shellcorona.cpp"

