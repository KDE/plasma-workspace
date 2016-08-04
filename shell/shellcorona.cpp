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
#include <kdirwatch.h>

#include <KPackage/PackageLoader>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

#include "config-ktexteditor.h" // HAVE_KTEXTEDITOR

#include "alternativeshelper.h"
#include "desktopview.h"
#include "panelview.h"
#include "scripting/scriptengine.h"
#include "plasmaquick/configview.h"
#include "shellmanager.h"
#include "osd.h"
#include "screenpool.h"
#include "waylanddialogfilter.h"

#include "plasmashelladaptor.h"

#include "futureutil.h"

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
      m_screenPool(new ScreenPool(KSharedConfig::openConfig(), this)),
      m_activityController(new KActivities::Controller(this)),
      m_addPanelAction(nullptr),
      m_addPanelsMenu(nullptr),
      m_interactiveConsole(nullptr),
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

    KDirWatch::self()->addFile(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1Char('/') + QStringLiteral("kdeglobals"));
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &ShellCorona::updateLookAndFeelPackage);
    connect(KDirWatch::self(), &KDirWatch::created, this, &ShellCorona::updateLookAndFeelPackage);

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
    m_lnfDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(m_lookAndFeelPackage.filePath("defaults")), "Desktop");
    m_lnfDefaultsConfig = KConfigGroup(&m_lnfDefaultsConfig, QStringLiteral("org.kde.plasma.desktop"));

    new PlasmaShellAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/PlasmaShell"), this);

    connect(this, &Plasma::Corona::startupCompleted, this,
            [this]() {
                qDebug() << "Plasma Shell startup completed";
                QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                               QStringLiteral("/KSplash"),
                                               QStringLiteral("org.kde.KSplash"),
                                               QStringLiteral("setStage"));
                ksplashProgressMessage.setArguments(QList<QVariant>() << QStringLiteral("desktop"));
                QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
                //TODO: remove
            });

    // Look for theme config in plasmarc, if it isn't configured, take the theme from the
    // LookAndFeel package, if either is set, change the default theme

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
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

    connect(m_activityController, &KActivities::Controller::currentActivityChanged, this, &ShellCorona::currentActivityChanged);
    connect(m_activityController, &KActivities::Controller::activityAdded, this, &ShellCorona::activityAdded);
    connect(m_activityController, &KActivities::Controller::activityRemoved, this, &ShellCorona::activityRemoved);

    new Osd(this);

    qApp->installEventFilter(this);
}

ShellCorona::~ShellCorona()
{
    while (!containments().isEmpty()) {
        //deleting a containment will remove it from the list due to QObject::destroyed connect in Corona
        delete containments().first();
    }
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
    m_lnfDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(m_lookAndFeelPackage.filePath("defaults")), "Desktop");
    m_lnfDefaultsConfig = KConfigGroup(&m_lnfDefaultsConfig, shell);

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
     * load() will check that we have a kscreen config, and m_activityController->serviceStatus() is not loading (i.e not unknown)
     *
     * It might seem that we only need this connection if the activityConsumer is currently in state Unknown, however
     * there is an issue where m_activityController will start the kactivitymanagerd, as KAMD is starting the serviceStatus will be "not running"
     * Whilst we are loading the kscreen config, the event loop runs and we might find KAMD has started.
     * m_activityController will change from "not running" to unknown, and might still be unknown when the kscreen fetching is complete.
     *
     * if that happens we want to continue monitoring for state changes, and only finally load when it is up.
     *
     * See https://bugs.kde.org/show_bug.cgi?id=342431 be careful about changing
     *
     * The unique connection makes sure we don't reload plasma if KAMD ever crashes and reloads, the signal is disconnected in the body of load
     */

    connect(m_activityController, &KActivities::Controller::serviceStatusChanged, this, &ShellCorona::load, Qt::UniqueConnection);

    if (m_activityController->serviceStatus() == KActivities::Controller::Running) {
        load();
    }
}


QString dumpconfigGroupJS(const KConfigGroup &rootGroup, const QString &prefix)
{
    QString script;
    QStringList hierarchy;
    QList<KConfigGroup> groups;
    groups << rootGroup;
    QSet<QString> visitedNodes;

    //perform a depth-first tree traversal for config groups
    while (!groups.isEmpty()) {
        KConfigGroup cg = groups.last();

        KConfigGroup parentCg = cg;
        //FIXME: name is not enoug
        hierarchy.clear();
        while (parentCg.isValid() && parentCg.name() != rootGroup.name()) {
            hierarchy.prepend(parentCg.name());
            parentCg = parentCg.parent();
        }

        visitedNodes.insert(hierarchy.join(QChar()));
        groups.pop_back();

        if (!cg.keyList().isEmpty()) {
            script += "\n";
            //TODO: this is conditional if applet or containment
            if (hierarchy.length() > 0) {
                 script += QStringLiteral("    //all config values in the \"%1\" group\n").arg(hierarchy.last());
                script += prefix + QStringLiteral(".currentConfigGroup = Array(\"") + hierarchy.join("\", \"") + "\");\n";
            }

            QMap<QString, QString>::const_iterator i;
            QMap<QString, QString> map = cg.entryMap();
            for (i = map.constBegin(); i != map.constEnd(); ++i) {
                //some blacklisted keys we don't want to save
                if (i.key() != QStringLiteral("activityId") || i.key() != QStringLiteral("ItemsGeometries")) {
                    script += prefix + ".writeConfig(\"" + i.key() + "\", \"" + i.value() + "\");\n";
                }
            }
        }

        foreach (const QString &groupName, cg.groupList()) {
            if (groupName == QStringLiteral("Applets") ||
                visitedNodes.contains(hierarchy.join(QChar()) + groupName)) {
                continue;
            }
            groups << KConfigGroup(&cg, groupName);
        }
    }

    return script;
}

QString ShellCorona::dumpCurrentLayoutJS()
{
    QString script;

    //dump desktop containments
    script += "//////Desktops\n";
    script += "var desktopsArray = desktopsForActivity(currentActivity());\n";
    //enumerate containments
    int i = 0;
    foreach (DesktopView *view, m_desktopViewforId.values()) {
        Plasma::Containment *cont = view->containment();
        script += "if (desktopsArray.length > " + QString::number(i) + ") {\n";
        script += "    var cont = desktopsArray[" + QString::number(i) + "];\n\n";
        script += "    cont.wallpaperPlugin = '" + cont->wallpaper() + "';\n";

        //enumerate config keys for containment
        KConfigGroup contConfig = cont->config();
        script += "    //Containment configuration\n";
        script += dumpconfigGroupJS(contConfig, QStringLiteral("    cont"));
        
        //try to parse the items geometries
        KConfigGroup genericConf(&contConfig, QStringLiteral("General"));
        const QStringList appletsGeomStrings = genericConf.readEntry(QStringLiteral("ItemsGeometries"), QString()).split(QChar(';'));

        QHash<QString, QRect> appletGeometries;
        for (const QString encoded : appletsGeomStrings) {
            const QStringList keyValue = encoded.split(QChar(':'));
            if (keyValue.length() != 2) {
                continue;
            }
            QStringList rectPieces = keyValue.last().split(QChar(','));
            if (rectPieces.length() != 5) {
                continue;
            }
            QRect rect(rectPieces[0].toInt(), rectPieces[1].toInt(),
                       rectPieces[2].toInt(), rectPieces[3].toInt());
            appletGeometries[keyValue.first()] = rect;
        }

        script += "\n\n";
        foreach (Plasma::Applet *applet, cont->applets()) {
            const QRect geometry = appletGeometries.value(QStringLiteral("Applet-") % QString::number(applet->id()));
            script += "    {\n";
            script += "        //Configuration of applet " + applet->title() + "\n";
            script += "        var applet = cont.addWidget(\"" + applet->pluginInfo().pluginName() + "\", {\"x\": " + QString::number(geometry.x()) + ",\"y\"; " + QString::number(geometry.y()) + ",\"width\": " + QString::number(geometry.width()) + ", \"height\": " + QString::number(geometry.height()) + "});\n";

            KConfigGroup appletConfig = applet->config();
            script += dumpconfigGroupJS(appletConfig, QStringLiteral("        applet"));

            script += "    }\n";
        }
        ++i;
        script += "}\n\n";
    }

    //same gridUnit calculation as ScriptEngine
    int gridUnit = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();
    if (gridUnit % 2 != 0) {
        gridUnit++;
    }
    //dump panels
    QHash<const Plasma::Containment *, PanelView *>::const_iterator it;
    for (it = m_panelViews.constBegin(); it != m_panelViews.constEnd(); ++it) {
        const Plasma::Containment *cont = it.key();
        const PanelView *view = it.value();

        script += "//////Panels\n";
        script += "{\n";
        script += "    var panel = new Panel;\n";
        qreal units = 1;

        switch (cont->location()) {
        case Plasma::Types::TopEdge:
            script += "    panel.location = \"top\";\n";
            units = view->height() / gridUnit;
            break;
        case Plasma::Types::LeftEdge:
            script += "    panel.location = \"left\";\n";
            units = view->width() / gridUnit;
            break;
        case Plasma::Types::RightEdge:
            script += "    panel.location = \"right\";\n";
            units = view->width() / gridUnit;
            break;
        case Plasma::Types::BottomEdge:
        default:
            script += "    panel.location = \"bottom\";\n";
            units = view->height() / gridUnit;
            break;
        }
        script += "    panel.height = gridUnit * " + QString::number(units) + ";\n";

        //enumerate config keys for containment
        KConfigGroup contConfig = cont->config();
        script += "    //Panel containment configuration\n";
        script += dumpconfigGroupJS(contConfig, QStringLiteral("    panel"));
        script += "\n\n";

        //try to parse the encoded applets order
        KConfigGroup genericConf(&contConfig, QStringLiteral("General"));
        const QStringList appletsOrderStrings = genericConf.readEntry(QStringLiteral("AppletOrder"), QString()).split(QChar(';'));
        //consider the applet order to be valid only if there are as many entries as applets()
        if (appletsOrderStrings.length() == cont->applets().length()) {
            foreach (const QString &stringId, appletsOrderStrings) {
                KConfigGroup appletConfig(&contConfig, QStringLiteral("Applets"));
                appletConfig = KConfigGroup(&appletConfig, stringId);
                const QString pluginName = appletConfig.readEntry(QStringLiteral("plugin"), QString());

                if (pluginName.isEmpty()) {
                    continue;
                }

                script += "    {\n";
                script += "        //Configuration of applet " + pluginName + "\n";
                script += "        var applet = panel.addWidget(\"" + pluginName + "\");\n";

                script += dumpconfigGroupJS(appletConfig, QStringLiteral("        applet"));

                script += "    }\n";
            }
        //applets order not found, just use order returned by applets()
        } else {
            foreach (Plasma::Applet *applet, cont->applets()) {
                script += "    {\n";
                script += "        //Configuration of applet " + applet->pluginInfo().pluginName() + "\n";
                script += "        var applet = panel.addWidget(\"" + applet->pluginInfo().pluginName() + "\", " + QString::number(applet->id()) + ");\n";

                KConfigGroup appletConfig = applet->config();
                script += dumpconfigGroupJS(appletConfig, QStringLiteral("        applet"));

                script += "    }\n";
            }
        }

        script += "}\n";
        script += "\n\n";
    }

    return script;
}

void ShellCorona::updateLookAndFeelPackage(const QString &file)
{
    //only care about kdeglobals
    if (!file.endsWith(QStringLiteral("kdeglobals"))) {
        return;
    }

    //TODO: put here anything that needs to update lnf-based
    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    if (packageName.isEmpty()) {
        return;
    }

    if (packageName == m_lookAndFeelPackage.metadata().pluginId()) {
        return;
    }

    m_lookAndFeelPackage.setPath(packageName);

    //NOTE: updateng the plasma theme should *not* be necessary here as the kcm is already doing this
    unload();
    load();
}

QString ShellCorona::shell() const
{
    return m_shell;
}

void ShellCorona::load()
{
    if (m_shell.isEmpty() ||
        m_activityController->serviceStatus() != KActivities::Controller::Running) {
        return;
    }

    disconnect(m_activityController, &KActivities::Controller::serviceStatusChanged, this, &ShellCorona::load);

    QString configFileName("plasma-" + m_shell);
    //NOTE: this is for retrocompatibility: keep who is using the default lnf package
    //the old config file name
    //TODO:alternative: kconfigupdate?
    if (m_lookAndFeelPackage.metadata().pluginId() != "org.kde.breeze.desktop") {
        configFileName += "-" + m_lookAndFeelPackage.metadata().pluginId();
    }
    configFileName += "-appletsrc";

    loadLayout(configFileName);

    checkActivities();

    if (containments().isEmpty()) {
        // Seems like we never really get to this point since loadLayout already
        // (virtually) calls loadDefaultLayout if it does not load anything
        // from the config file. Maybe if the config file is not empty,
        // but still does not have any containments
        loadDefaultLayout();
        processUpdateScripts();
    } else {
        processUpdateScripts();
        foreach(Plasma::Containment *containment, containments()) {
            if (containment->containmentType() == Plasma::Types::PanelContainment || containment->containmentType() == Plasma::Types::CustomPanelContainment) {
                //Don't give a view to containments that don't want one (negative lastscreen)
                //this is pretty mucha special case for the systray
                if (!m_waitingPanels.contains(containment) && containment->lastScreen() >= 0) {
                    m_waitingPanels << containment;
                }
            //historically CustomContainments are treated as desktops
            } else if (containment->containmentType() == Plasma::Types::DesktopContainment || containment->containmentType() == Plasma::Types::CustomContainment) {
                //FIXME ideally fix this, or at least document the crap out of it
                int screen = containment->lastScreen();
                if (screen < 0) {
                    screen = 0;
                    qWarning() << "last screen is < 0 so putting containment on screen " << screen;
                }
                insertContainment(containment->activity(), screen, containment);
            }
        }
    }

    //NOTE: this is needed in case loadLayout() did *not* call loadDefaultLayout()
    //it needs to be after of loadLayout() as it would always create new
    //containments on each startup otherwise
    for (QScreen* screen : qGuiApp->screens()) {
        addOutput(screen);
    }
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ShellCorona::addOutput);
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &ShellCorona::primaryOutputChanged);
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
    if (!m_desktopViewforId.contains(0)) {
        return;
    }

    QScreen *oldPrimary = m_desktopViewforId.value(0)->screen();
    QScreen *newPrimary = qGuiApp->primaryScreen();
    if (!newPrimary || newPrimary == oldPrimary) {
        return;
    }

    qWarning()<<"Old primary output:"<<oldPrimary<<"New primary output:"<<newPrimary;
    const int oldIdOfPrimary = m_screenPool->id(newPrimary->name());
    //swap order in m_desktopViewforId
    if (m_desktopViewforId.contains(0) && m_desktopViewforId.contains(oldIdOfPrimary)) {
        DesktopView *oldPrimaryDesktop = m_desktopViewforId.value(0);
        DesktopView *newPrimaryDesktop = m_desktopViewforId.value(oldIdOfPrimary);

        oldPrimaryDesktop->setScreenToFollow(newPrimaryDesktop->screenToFollow());
        newPrimaryDesktop->setScreenToFollow(newPrimary);
        m_desktopViewforId[0] = newPrimaryDesktop;
        m_desktopViewforId[oldIdOfPrimary] = oldPrimaryDesktop;
    }
    m_screenPool->setPrimaryConnector(newPrimary->name());

    foreach (PanelView *panel, m_panelViews) {
        if (panel->screen() == oldPrimary) {
            panel->setScreenToFollow(newPrimary);
        } else if (panel->screen() == newPrimary) {
            panel->setScreenToFollow(oldPrimary);
        }
    }

    CHECK_SCREEN_INVARIANTS
}

#ifndef NDEBUG
void ShellCorona::screenInvariants() const
{
    Q_ASSERT(m_desktopViewforId.keys().count() <= QGuiApplication::screens().count());
    QScreen *s = !m_desktopViewforId.contains(0) ? nullptr : m_desktopViewforId.value(0)->screen();

    QScreen* ks = qGuiApp->primaryScreen();
    Q_ASSERT(ks == s);

    QSet<QScreen*> screens;
    foreach (const int id, m_desktopViewforId.keys()) {
        const DesktopView *view = m_desktopViewforId.value(id);
        QScreen *screen = view->screen();
        Q_ASSERT(!screens.contains(screen));
        Q_ASSERT(!m_redundantOutputs.contains(screen));
//         commented out because a different part of the code-base is responsible for this
//         and sometimes is not yet called here.
//         Q_ASSERT(!view->fillScreen() || view->geometry() == screen->geometry());
        Q_ASSERT(view->containment());
        Q_ASSERT(view->containment()->screen() == id || view->containment()->screen() == -1);
        Q_ASSERT(view->containment()->lastScreen() == id || view->containment()->lastScreen() == -1);
        Q_ASSERT(view->isVisible());

        foreach (const PanelView *panel, panelsForScreen(screen)) {
            Q_ASSERT(panel->containment());
            Q_ASSERT(panel->containment()->screen() == id || panel->containment()->screen() == -1);
            Q_ASSERT(panel->isVisible());
        }

        screens.insert(screen);
    }

    foreach (QScreen* out, m_redundantOutputs) {
        Q_ASSERT(isOutputRedundant(out));
    }

    if (m_desktopViewforId.isEmpty()) {
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
    qDeleteAll(m_desktopViewforId.values());
    m_desktopViewforId.clear();
    qDeleteAll(m_panelViews.values());
    m_panelViews.clear();
    m_desktopContainments.clear();
    m_waitingPanels.clear();
    m_activityContainmentPlugins.clear();

    while (!containments().isEmpty()) {
        //deleting a containment will remove it from the list due to QObject::destroyed connect in Corona
        //this form doesn't crash, while qDeleteAll(containments()) does
        delete containments().first()->property("_plasma_graphicObject").value<QObject *>();
        delete containments().first();
    }
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
    //NOTE: Is important the containments already exist for each screen
    // at the moment of the script execution,the same loop in :load()
    // is executed too late
    for (QScreen* screen : qGuiApp->screens()) {
        addOutput(screen);
    }

    QString script = ShellManager::s_testModeLayout;

    if (script.isEmpty()) {
        script = m_lookAndFeelPackage.filePath("layouts", QString(shell() + "-layout.js").toLatin1());
    }
    if (script.isEmpty()) {
        script = package().filePath("defaultlayout");
    }

    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QString code = file.readAll();
        qDebug() << "evaluating startup script:" << script;

        // We need to know which activities are here in order for
        // the scripting engine to work. activityAdded does not mind
        // if we pass it the same activity multiple times
        QStringList existingActivities = m_activityController->activities();
        foreach (const QString &id, existingActivities) {
            activityAdded(id);
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
        if (!scriptEngine.evaluateScript(code, script)) {
            qWarning() << "failed to initialize layout properly:" << script;
        }
    }

    Q_EMIT startupCompleted();
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

int ShellCorona::numScreens() const
{
    return qGuiApp->screens().count();
}

QRect ShellCorona::screenGeometry(int id) const
{
    if (!m_desktopViewforId.contains(id)) {
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = qGuiApp->primaryScreen();
        return s ? s->geometry() : QRect();
    }
    return m_desktopViewforId.value(id)->geometry();
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    if (!m_desktopViewforId.contains(id)) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = qGuiApp->primaryScreen();
        return s ? s->availableGeometry() : QRegion();
    }
    DesktopView *view = m_desktopViewforId.value(id);

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
    if (!m_desktopViewforId.contains(id)) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = qGuiApp->primaryScreen();
        return s ? s->availableGeometry() : QRect();
    }

    DesktopView *view = m_desktopViewforId.value(id);

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

QStringList ShellCorona::availableActivities() const
{
    return m_activityContainmentPlugins.keys();
}

void ShellCorona::removeDesktop(DesktopView *desktopView)
{
    const int idx = m_screenPool->id(desktopView->screenToFollow()->name());

    if (!m_desktopViewforId.contains(idx)) {
        return;
    }

    QMutableHashIterator<const Plasma::Containment *, PanelView *> it(m_panelViews);
    while (it.hasNext()) {
        it.next();
        PanelView *panelView = it.value();

        if (panelView->containment()->screen() == idx) {
            m_waitingPanels << panelView->containment();
            it.remove();
            delete panelView;
        }
    }

    delete m_desktopViewforId.value(idx);
    m_desktopViewforId.remove(idx);
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
    return m_desktopViewforId.value(m_screenPool->id(screen->name()));
}

void ShellCorona::screenRemoved(QScreen* screen)
{
    if (DesktopView* v = desktopForScreen(screen)) {
        removeDesktop(v);
    }

    m_reconsiderOutputsTimer.start();
    m_redundantOutputs.remove(screen);
}

bool ShellCorona::isOutputRedundant(QScreen* screen) const
{
    Q_ASSERT(screen);
    const QRect geometry = screen->geometry();

    //FIXME: QScreen doesn't have any idea of "this qscreen is clone of this other one
    //so this ultra inefficient heuristic has to stay until we have a slightly better api
    foreach (QScreen* s, qGuiApp->screens()) {
        if (screen == s) {
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
    foreach (QScreen* screen, qGuiApp->screens()) {
        if (m_redundantOutputs.contains(screen)) {
            if (!isOutputRedundant(screen)) {
//                 qDebug() << "not redundant anymore" << out;
                addOutput(screen);
            }
        } else if (isOutputRedundant(screen)) {
            qDebug() << "new redundant screen" << screen;

            if (DesktopView* v = desktopForScreen(screen))
                removeDesktop(v);

            m_redundantOutputs.insert(screen);
        }
//         else
//             qDebug() << "fine screen" << out;
    }

    updateStruts();

    CHECK_SCREEN_INVARIANTS
}

void ShellCorona::addOutput(QScreen* screen)
{
    Q_ASSERT(screen);

    connect(screen, &QScreen::geometryChanged,
            &m_reconsiderOutputsTimer, static_cast<void (QTimer::*)()>(&QTimer::start),
            Qt::UniqueConnection);

    if (isOutputRedundant(screen)) {
        m_redundantOutputs.insert(screen);
        return;
    } else {
        m_redundantOutputs.remove(screen);
    }

    int insertPosition = m_screenPool->id(screen->name());
    if (insertPosition < 0) {
        insertPosition = m_screenPool->firstAvailableId();
    }

    DesktopView *view = new DesktopView(this, screen);
    connect(view, &QQuickWindow::sceneGraphError, this, &ShellCorona::showOpenGLNotCompatibleWarning);

    Plasma::Containment *containment = createContainmentForActivity(m_activityController->currentActivity(), insertPosition);
    Q_ASSERT(containment);

    QAction *removeAction = containment->actions()->action(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }

    m_screenPool->insertScreenMapping(insertPosition, screen->name());
    m_desktopViewforId[insertPosition] = view;
    view->setContainment(containment);
    view->show();
    Q_ASSERT(screen == view->screen());

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

Plasma::Containment *ShellCorona::createContainmentForActivity(const QString& activity, int screenNum)
{
    if (m_desktopContainments.contains(activity)) {
        QHash<int, Plasma::Containment *> act = m_desktopContainments.value(activity);
        QHash<int, Plasma::Containment *>::const_iterator it = act.constFind(screenNum);
        if (it != act.constEnd() && (*it)) {
            return *it;
        }
    }

    QString plugin = m_activityContainmentPlugins.value(activity);

    if (plugin.isEmpty()) {
        plugin = defaultContainmentPlugin();
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
            requestedScreen = 0;
        }

        if (!m_desktopViewforId.contains(requestedScreen)) {
            stillWaitingPanels << cont;
            continue;
        }

        //TODO: does a similar check make sense?
        //Q_ASSERT(qBound(0, requestedScreen, m_screenPool->count() - 1) == requestedScreen);
        QScreen *screen = m_desktopViewforId.value(requestedScreen)->screenToFollow();
        PanelView* panel = new PanelView(this, screen);
        connect(panel, &QQuickWindow::sceneGraphError, this, &ShellCorona::showOpenGLNotCompatibleWarning);
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
    foreach (DesktopView *view, m_desktopViewforId.values()) {
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
    foreach (DesktopView *view, m_desktopViewforId.values()) {
        if (view->screen()->geometry().contains(cursorPos)) {
            //The view QML has to provide something to display the activity explorer
            view->rootObject()->metaObject()->invokeMethod(view->rootObject(), "toggleActivityManager", Qt::QueuedConnection);
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

void ShellCorona::loadInteractiveConsole()
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
}

void ShellCorona::showInteractiveConsole()
{
    loadInteractiveConsole();
    if (m_interactiveConsole && m_interactiveConsole->rootObject()) {
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

void ShellCorona::showInteractiveKWinConsole()
{
    loadInteractiveConsole();

    if (m_interactiveConsole && m_interactiveConsole->rootObject()) {
        m_interactiveConsole->rootObject()->setProperty("mode", "windowmanager");
        m_interactiveConsole->rootObject()->setProperty("visible", true);
    }
}

void ShellCorona::loadKWinScriptInInteractiveConsole(const QString &script)
{
    showInteractiveKWinConsole();
    if (m_interactiveConsole) {
        m_interactiveConsole->rootObject()->setProperty("script", script);
    }
}

void ShellCorona::evaluateScript(const QString &script) {
    if (immutability() != Plasma::Types::Mutable) {
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::Failed, QStringLiteral("Widgets are locked"));
        }
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

    scriptEngine.evaluateScript(script);
    if (scriptEngine.hasUncaughtException() && calledFromDBus()) {
        sendErrorReply(QDBusError::Failed, scriptEngine.uncaughtException().toString());
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
    KActivities::Controller::ServiceStatus status = m_activityController->serviceStatus();
    //qDebug() << "$%$%$#%$%$%Status:" << status;
    if (status != KActivities::Controller::Running) {
        //panic and give up - better than causing a mess
        qDebug() << "ShellCorona::checkActivities is called whilst activity daemon is still connecting";
        return;
    }

    QStringList existingActivities = m_activityController->activities();
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

    foreach (int id, m_desktopViewforId.keys()) {
        Plasma::Containment *c = createContainmentForActivity(newActivity, id);

        QAction *removeAction = c->actions()->action(QStringLiteral("remove"));
        if (removeAction) {
            removeAction->deleteLater();
        }
        m_desktopViewforId.value(id)->setContainment(c);
    }
}

void ShellCorona::activityAdded(const QString &id)
{
    //TODO more sanity checks
    if (m_activityContainmentPlugins.contains(id)) {
        qWarning() << "Activity added twice" << id;
        return;
    }

    m_activityContainmentPlugins.insert(id, defaultContainmentPlugin());
}

void ShellCorona::activityRemoved(const QString &id)
{
    m_activityContainmentPlugins.remove(id);
}

void ShellCorona::insertActivity(const QString &id, const QString &plugin)
{
    activityAdded(id);

    const QString currentActivityReally = m_activityController->currentActivity();

    // TODO: This needs to go away!
    // The containment creation API does not know when we have a
    // new activity to create a containment for, we need to pretend
    // that the current activity has been changed
    QFuture<bool> currentActivity = m_activityController->setCurrentActivity(id);
    awaitFuture(currentActivity);

    if (!currentActivity.result()) {
        qDebug() << "Failed to create and switch to the activity";
        return;
    }

    while (m_activityController->currentActivity() != id) {
        QCoreApplication::processEvents();
    }

    m_activityContainmentPlugins.insert(id, plugin);
    foreach (int screenId, m_desktopViewforId.keys()) {
        Plasma::Containment *c = createContainmentForActivity(id, screenId);
        if (c) {
            c->config().writeEntry("lastScreen", screenId);
        }
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
    foreach (DesktopView *v, m_desktopViewforId.values()) {
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
            KPluginInfo info(pair.second);
            package.setPath(info.pluginName());
            const QString scriptFile = package.filePath("mainscript");
            if (!scriptFile.isEmpty()) {
                QAction *action = m_addPanelsMenu->addAction(info.name());
                action->setData(QStringLiteral("plasma-desktop-template:%1").arg(info.pluginName()));
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
        const QString templateName = plugin.right(plugin.length() - qstrlen("plasma-desktop-template:"));

        scriptEngine.evaluateScript(QStringLiteral("loadTemplate(\"%1\")").arg(templateName));
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
            m_panelViews[panel]->setScreenToFollow(screen);
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
    foreach (int id, m_desktopViewforId.keys()) {
        if (m_desktopViewforId.value(id)->containment() == containment && containment->activity() == m_activityController->currentActivity()) {
            return id;
        }
    }

    //if the panel views already exist, base upon them
    PanelView *view = m_panelViews.value(containment);
    if (view) {
        return m_screenPool->id(view->screenToFollow()->name());
    }

    //Failed? fallback on lastScreen()
    //lastScreen() is the correct screen for panels
    //It is also correct for desktops *that have the correct activity()*
    //a containment with lastScreen() == 0 but another activity,
    //won't be associated to a screen
//     qDebug() << "ShellCorona screenForContainment: " << containment << " Last screen is " << containment->lastScreen();
    for (int i = 0, count = qGuiApp->screens().count(); i<count; ++i) {
        if (containment->lastScreen() == i &&
            (containment->activity() == m_activityController->currentActivity() ||
            containment->containmentType() == Plasma::Types::PanelContainment || containment->containmentType() == Plasma::Types::CustomPanelContainment)) {
            return i;
        }
    }

    return -1;
}

void ShellCorona::nextActivity()
{
    const QStringList list = m_activityController->activities(KActivities::Info::Running);
    if (list.isEmpty()) {
        return;
    }

    const int start = list.indexOf(m_activityController->currentActivity());
    const int i = (start + 1) % list.size();

    m_activityController->setCurrentActivity(list.at(i));
}

void ShellCorona::previousActivity()
{
    const QStringList list = m_activityController->activities(KActivities::Info::Running);
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
    const QStringList list = m_activityController->activities(KActivities::Info::Running);
    if (list.isEmpty()) {
        return;
    }

    m_activityController->stopActivity(m_activityController->currentActivity());
}

void ShellCorona::insertContainment(const QString &activity, int screenNum, Plasma::Containment *containment)
{
    Plasma::Containment *cont = m_desktopContainments.value(activity).value(screenNum);
    if (containment == cont) {
        return;
    }

    Q_ASSERT(!m_desktopContainments.value(activity).values().contains(containment));

    if (cont) {
        disconnect(cont, SIGNAL(destroyed(QObject*)),
                this, SLOT(desktopContainmentDestroyed(QObject*)));
        cont->destroy();
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

void ShellCorona::showOpenGLNotCompatibleWarning()
{
    static bool s_multipleInvokations = false;
    if (s_multipleInvokations) {
        return;
    }
    s_multipleInvokations = true;

    QCoreApplication::setAttribute(Qt::AA_ForceRasterWidgets);
    QMessageBox::critical(nullptr, i18n("Plasma Failed To Start"),
                          i18n("Plasma is unable to start as it could not correctly use OpenGL 2.\n Please check that your graphic drivers are set up correctly."));
    qCritical("Open GL context could not be created");

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

ScreenPool *ShellCorona::screenPool() const
{
    return m_screenPool;
}

QList<int> ShellCorona::screenIds() const
{
    return m_desktopViewforId.keys();
}

QString ShellCorona::defaultContainmentPlugin() const
{
    QString plugin = m_lnfDefaultsConfig.readEntry("Containment", QString());
    if (plugin.isEmpty()) {
        plugin = m_desktopDefaultsConfig.readEntry("Containment", "org.kde.desktopcontainment");
    }
    return plugin;
}

void ShellCorona::updateStruts()
{
    foreach(PanelView* view, m_panelViews) {
        view->updateStruts();
    }
}

// Desktop corona handler


#include "moc_shellcorona.cpp"

