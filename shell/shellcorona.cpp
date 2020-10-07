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
#include "strutmanager.h"

#include <config-plasma.h>

#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QQmlContext>
#include <QDBusConnection>
#include <QUrl>
#include <QScreen>

#include <QJsonObject>
#include <QJsonDocument>

#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <Plasma/Package>
#include <Plasma/PluginLoader>
#include <PlasmaQuick/Dialog>

#include <kactivities/controller.h>
#include <kactivities/consumer.h>
#include <ksycoca.h>
#include <KGlobalAccel>
#include <KAuthorized>
#include <KWindowSystem>
#include <kdeclarative/kdeclarative.h>
#include <kdeclarative/qmlobjectsharedengine.h>
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
#include "osd.h"
#include "screenpool.h"

#include "plasmashelladaptor.h"
#include "debug.h"
#include "futureutil.h"

#ifndef NDEBUG
    #define CHECK_SCREEN_INVARIANTS screenInvariants();
#else
    #define CHECK_SCREEN_INVARIANTS
#endif

#if HAVE_X11
#include <NETWM>
#include <QX11Info>
#include <xcb/xcb.h>
#endif

static const int s_configSyncDelay = 10000; // 10 seconds

ShellCorona::ShellCorona(QObject *parent)
    : Plasma::Corona(parent),
      m_config(KSharedConfig::openConfig(QStringLiteral("plasmarc"))),
      m_screenPool(new ScreenPool(KSharedConfig::openConfig(), this)),
      m_activityController(new KActivities::Controller(this)),
      m_addPanelAction(nullptr),
      m_addPanelsMenu(nullptr),
      m_interactiveConsole(nullptr),
      m_waylandPlasmaShell(nullptr),
      m_closingDown(false),
      m_strutManager(new StrutManager(this))
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
    //we want our application config with screen mapping to always be in sync with the applets one, so a crash at any time will still
    //leave containments pointing to the correct screens
    connect(this, &Corona::configSynced, this, &ShellCorona::syncAppConfig);

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

    // Look for theme config in plasmarc, if it isn't configured, take the theme from the
    // LookAndFeel package, if either is set, change the default theme

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        //saveLayout is a slot but arguments not compatible
        m_closingDown = true;
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
    activityAction->setText(i18n("Show Activity Switcher"));
    activityAction->setIcon(QIcon::fromTheme(QStringLiteral("activities")));
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

    QAction *previousActivityAction = actions()->addAction(QStringLiteral("switch to previous activity"));
    connect(previousActivityAction, &QAction::triggered,
            this, &ShellCorona::previousActivity);
    previousActivityAction->setText(i18n("Switch to Previous Activity"));
    previousActivityAction->setData(Plasma::Types::ConfigureAction);
    previousActivityAction->setShortcutContext(Qt::ApplicationShortcut);

    KGlobalAccel::self()->setGlobalShortcut(previousActivityAction, QKeySequence());

    QAction *nextActivityAction = actions()->addAction(QStringLiteral("switch to next activity"));
    connect(nextActivityAction, &QAction::triggered,
            this, &ShellCorona::nextActivity);
    nextActivityAction->setText(i18n("Switch to Next Activity"));
    nextActivityAction->setData(Plasma::Types::ConfigureAction);
    nextActivityAction->setShortcutContext(Qt::ApplicationShortcut);

    KGlobalAccel::self()->setGlobalShortcut(nextActivityAction, QKeySequence());

    connect(m_activityController, &KActivities::Controller::currentActivityChanged, this, &ShellCorona::currentActivityChanged);
    connect(m_activityController, &KActivities::Controller::activityAdded, this, &ShellCorona::activityAdded);
    connect(m_activityController, &KActivities::Controller::activityRemoved, this, &ShellCorona::activityRemoved);

    KActionCollection *taskbarActions = new KActionCollection(this);
    for (int i = 0; i < 10; ++i) {
        const int entryNumber = i + 1;
        const Qt::Key key = static_cast<Qt::Key>(Qt::Key_0 + (entryNumber % 10));

        QAction *action = taskbarActions->addAction(QStringLiteral("activate task manager entry %1").arg(QString::number(entryNumber)));
        action->setText(i18n("Activate Task Manager Entry %1", entryNumber));
        KGlobalAccel::setGlobalShortcut(action, QKeySequence(Qt::META + key));
        connect(action, &QAction::triggered, this, [this, i] {
            activateTaskManagerEntry(i);
        });
    }

    new Osd(m_config, this);

    // catch when plasmarc changes, so we e.g. enable/disable the OSd
    m_configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1Char('/') + m_config->name();
    KDirWatch::self()->addFile(m_configPath);
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &ShellCorona::configurationChanged);
    connect(KDirWatch::self(), &KDirWatch::created, this, &ShellCorona::configurationChanged);

    connect(qApp, &QGuiApplication::focusWindowChanged,
            this, [this] (QWindow *focusWindow) {
            if (!focusWindow) {
                setEditMode(false);
            }
        }
    );
    connect(this, &ShellCorona::editModeChanged,
            this, [this](bool edit) {
                setDashboardShown(edit);
            }
    );
}

ShellCorona::~ShellCorona()
{
    while (!containments().isEmpty()) {
        // Deleting a containment will remove it from the list due to QObject::destroyed connect in Corona
        // Deleting a containment in turn also kills any panel views
        delete containments().first();
    }
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

    KConfigGroup plasmarc(m_config, themeGroupKey);
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

    load();
}

QJsonObject dumpconfigGroupJS(const KConfigGroup &rootGroup)
{
    QJsonObject result;

    QStringList hierarchy;
    QStringList escapedHierarchy;
    QList<KConfigGroup> groups{rootGroup};
    QSet<QString> visitedNodes;

    const QSet<QString> forbiddenKeys {
            QStringLiteral("activityId"),
            QStringLiteral("ItemsGeometries"),
            QStringLiteral("AppletOrder"),
            QStringLiteral("SystrayContainmentId"),
            QStringLiteral("location"),
            QStringLiteral("plugin")
        };

    auto groupID = [&escapedHierarchy]() {
        return '/' + escapedHierarchy.join('/');
    };

    // Perform a depth-first tree traversal for config groups
    while (!groups.isEmpty()) {
        KConfigGroup cg = groups.last();

        KConfigGroup parentCg = cg;
        //FIXME: name is not enough

        hierarchy.clear();
        escapedHierarchy.clear();
        while (parentCg.isValid() && parentCg.name() != rootGroup.name()) {
            const auto name = parentCg.name();
            hierarchy.prepend(name);
            escapedHierarchy.prepend(QString::fromUtf8(QUrl::toPercentEncoding(name.toUtf8())));
            parentCg = parentCg.parent();
        }

        visitedNodes.insert(groupID());
        groups.pop_back();

        QJsonObject configGroupJson;

        if (!cg.keyList().isEmpty()) {
            //TODO: this is conditional if applet or containment

            const auto map = cg.entryMap();
            auto i = map.cbegin();
            for (; i != map.cend(); ++i) {
                //some blacklisted keys we don't want to save
                if (!forbiddenKeys.contains(i.key())) {
                    configGroupJson.insert(i.key(), i.value());
                }
            }
        }

        const auto groupList = cg.groupList();
        for (const QString &groupName : groupList) {
            if (groupName == QLatin1String("Applets") ||
                visitedNodes.contains(groupID() + '/' + groupName)) {
                continue;
            }
            groups << KConfigGroup(&cg, groupName);
        }

        if (!configGroupJson.isEmpty()) {
            result.insert(groupID(), configGroupJson);
        }
    }

    return result;
}

QByteArray ShellCorona::dumpCurrentLayoutJS() const
{
    QJsonObject root;
    root.insert("serializationFormatVersion", "1");

    //same gridUnit calculation as ScriptEngine
    int gridUnit = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();
    if (gridUnit % 2 != 0) {
        gridUnit++;
    }

    auto isPanel = [] (Plasma::Containment *cont) {
        return
            (cont->formFactor() == Plasma::Types::Horizontal
                || cont->formFactor() == Plasma::Types::Vertical) &&
            (cont->location() == Plasma::Types::TopEdge
                || cont->location() == Plasma::Types::BottomEdge
                || cont->location() == Plasma::Types::LeftEdge
                || cont->location() == Plasma::Types::RightEdge) &&
            cont->pluginMetaData().pluginId() != QLatin1String("org.kde.plasma.private.systemtray");
    };

    auto isDesktop = [] (Plasma::Containment *cont) {
        return !cont->activity().isEmpty();
    };

    const auto containments = ShellCorona::containments();

    // Collecting panels

    QJsonArray panelsJsonArray;

    for (Plasma::Containment *cont : containments) {
        if (!isPanel(cont)) {
            continue;
        }

        QJsonObject panelJson;

        const PanelView *view = m_panelViews.value(cont);
        const auto location = cont->location();

        panelJson.insert("location",
                location == Plasma::Types::TopEdge   ? "top"
              : location == Plasma::Types::LeftEdge  ? "left"
              : location == Plasma::Types::RightEdge ? "right"
              : /* Plasma::Types::BottomEdge */        "bottom");

        const qreal height =
                // If we do not have a panel, fallback to 4 units
                !view     ?  4
              : (qreal)view->thickness() / gridUnit;

        panelJson.insert("height", height);
        if (view) {
            const auto alignment = view->alignment();
            panelJson.insert("maximumLength", (qreal)view->maximumLength() / gridUnit);
            panelJson.insert("minimumLength", (qreal)view->minimumLength() / gridUnit);
            panelJson.insert("offset", (qreal)view->offset() / gridUnit);
            panelJson.insert("alignment",
                 alignment == Qt::AlignRight  ? "right"
               : alignment == Qt::AlignCenter ? "center"
               : "left");
            switch (view->visibilityMode()) {
            case PanelView::AutoHide:
                panelJson.insert("hiding", "autohide");
                break;
            case PanelView::LetWindowsCover:
                panelJson.insert("hiding", "windowscover");
                break;
            case PanelView::WindowsGoBelow:
                panelJson.insert("hiding", "windowsbelow");
                break;
            case PanelView::NormalPanel:
            default:
                panelJson.insert("hiding", "normal");
                break;
            }
        }

        // Saving the config keys
        const KConfigGroup contConfig = cont->config();

        panelJson.insert("config", dumpconfigGroupJS(contConfig));

        // Generate the applets array
        QJsonArray appletsJsonArray;

        // Try to parse the encoded applets order
        const KConfigGroup genericConf(&contConfig, QStringLiteral("General"));
        const QStringList appletsOrderStrings =
            genericConf.readEntry(QStringLiteral("AppletOrder"), QString())
                .split(QChar(';'));

        // Consider the applet order to be valid only if there are as many entries as applets()
        if (appletsOrderStrings.length() == cont->applets().length()) {
            for (const QString &appletId : appletsOrderStrings) {
                KConfigGroup appletConfig(&contConfig, QStringLiteral("Applets"));
                appletConfig = KConfigGroup(&appletConfig, appletId);

                const QString pluginName =
                    appletConfig.readEntry(QStringLiteral("plugin"), QString());

                if (pluginName.isEmpty()) {
                    continue;
                }

                QJsonObject appletJson;

                appletJson.insert("plugin", pluginName);
                appletJson.insert("config", dumpconfigGroupJS(appletConfig));

                appletsJsonArray << appletJson;
            }

        } else {
            const auto applets = cont->applets();
            for (Plasma::Applet *applet : applets) {
                QJsonObject appletJson;

                KConfigGroup appletConfig = applet->config();

                appletJson.insert("plugin", applet->pluginMetaData().pluginId());
                appletJson.insert("config", dumpconfigGroupJS(appletConfig));

                appletsJsonArray << appletJson;
            }
        }

        panelJson.insert("applets", appletsJsonArray);

        panelsJsonArray << panelJson;
    }

    root.insert("panels", panelsJsonArray);

    // Now we are collecting desktops

    QJsonArray desktopsJson;

    const auto currentActivity = m_activityController->currentActivity();

    for (Plasma::Containment *cont : containments) {
        if (!isDesktop(cont) || cont->activity() != currentActivity) {
            continue;
        }

        QJsonObject desktopJson;

        desktopJson.insert("wallpaperPlugin", cont->wallpaper());

        // Get the config for the containment
        KConfigGroup contConfig = cont->config();
        desktopJson.insert("config", dumpconfigGroupJS(contConfig));

        // Try to parse the item geometries
        const KConfigGroup genericConf(&contConfig, QStringLiteral("General"));
        const QStringList appletsGeomStrings =
            genericConf.readEntry(QStringLiteral("ItemsGeometries"), QString())
                .split(QChar(';'));


        QHash<QString, QRect> appletGeometries;
        for (const QString &encoded : appletsGeomStrings) {
            const QStringList keyValue = encoded.split(QChar(':'));
            if (keyValue.length() != 2) {
                continue;
            }

            const QStringList rectPieces = keyValue.last().split(QChar(','));
            if (rectPieces.length() != 5) {
                continue;
            }

            QRect rect(rectPieces[0].toInt(), rectPieces[1].toInt(),
                       rectPieces[2].toInt(), rectPieces[3].toInt());

            appletGeometries[keyValue.first()] = rect;
        }

        QJsonArray appletsJsonArray;

        const auto applets = cont->applets();
        for (Plasma::Applet *applet : applets) {
            const QRect geometry = appletGeometries.value(
                QStringLiteral("Applet-") % QString::number(applet->id()));

            QJsonObject appletJson;

            appletJson.insert("title",      applet->title());
            appletJson.insert("plugin",     applet->pluginMetaData().pluginId());

            appletJson.insert("geometry.x",      geometry.x() / gridUnit);
            appletJson.insert("geometry.y",      geometry.y() / gridUnit);
            appletJson.insert("geometry.width",  geometry.width() / gridUnit);
            appletJson.insert("geometry.height", geometry.height() / gridUnit);

            KConfigGroup appletConfig = applet->config();
            appletJson.insert("config", dumpconfigGroupJS(appletConfig));

            appletsJsonArray << appletJson;
        }

        desktopJson.insert("applets", appletsJsonArray);
        desktopsJson << desktopJson;
    }

    root.insert("desktops", desktopsJson);

    QJsonDocument json;
    json.setObject(root);

    return
        "var plasma = getApiVersion(1);\n\n"
        "var layout = " + json.toJson() + ";\n\n"
        "plasma.loadSerializedLayout(layout);\n";
}

void ShellCorona::loadLookAndFeelDefaultLayout(const QString &packageName)
{
    KPackage::Package newPack = m_lookAndFeelPackage;
    newPack.setPath(packageName);

    if (!newPack.isValid()) {
        return;
    }

    KSharedConfig::Ptr conf = KSharedConfig::openConfig(QLatin1String("plasma-") + m_shell + QLatin1String("-appletsrc"), KConfig::SimpleConfig);

    m_lookAndFeelPackage.setPath(packageName);

    //get rid of old config
    for (const QString &group : conf->groupList()) {
        conf->deleteGroup(group);
    }
    conf->sync();
    unload();
    // Put load in queue of the event loop to wait for the whole set of containments to have been deleteLater(), as some like FolderView operate on singletons which can cause inconsistent states
    QTimer::singleShot(0, this, &ShellCorona::load);
}

QString ShellCorona::shell() const
{
    return m_shell;
}

void ShellCorona::load()
{
    if (m_shell.isEmpty()) {
        return;
    }

    auto activityStatus = m_activityController->serviceStatus();
    if (activityStatus != KActivities::Controller::Running &&
        !qApp->property("org.kde.KActivities.core.disableAutostart").toBool()) {
        if (activityStatus == KActivities::Controller::NotRunning) {
            qWarning("Aborting shell load: The activity manager daemon (kactivitymanagerd) is not running.");
            qWarning("If this Plasma has been installed into a custom prefix, verify that its D-Bus services dir is known to the system for the daemon to be activatable.");
        }
        return;
    }

    disconnect(m_activityController, &KActivities::Controller::serviceStatusChanged, this, &ShellCorona::load);

    m_screenPool->load();

    //TODO: a kconf_update script is needed
    QString configFileName(QStringLiteral("plasma-") + m_shell + QStringLiteral("-appletsrc"));

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
        const auto containments = this->containments();
        for (Plasma::Containment *containment : containments) {
            if (containment->containmentType() == Plasma::Types::PanelContainment || containment->containmentType() == Plasma::Types::CustomPanelContainment) {
                //Don't give a view to containments that don't want one (negative lastscreen)
                //(this is pretty mucha special case for the systray)
                //also, make sure we don't have a view already.
                //this will be true for first startup as the view has already been created at the new Panel JS call
                if (!m_waitingPanels.contains(containment) && containment->lastScreen() >= 0 && !m_panelViews.contains(containment)) {
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
        //the containments may have been created already by the startup script
        //check their existence in order to not have duplicated desktopviews
        if (!m_desktopViewforId.contains(m_screenPool->id(screen->name()))) {
            addOutput(screen);
        }
    }
    connect(qGuiApp, &QGuiApplication::screenAdded, this, &ShellCorona::addOutput, Qt::UniqueConnection);
    connect(qGuiApp, &QGuiApplication::primaryScreenChanged, this, &ShellCorona::primaryOutputChanged, Qt::UniqueConnection);
    connect(qGuiApp, &QGuiApplication::screenRemoved, this, &ShellCorona::handleScreenRemoved, Qt::UniqueConnection);

    if (!m_waitingPanels.isEmpty()) {
        m_waitingPanelsTimer.start();
    }

    if (config()->isImmutable() ||
        !KAuthorized::authorize(QStringLiteral("plasma/plasmashell/unlockedDesktop"))) {
        setImmutability(Plasma::Types::SystemImmutable);
    } else {
        KConfigGroup coronaConfig(config(), "General");
        setImmutability((Plasma::Types::ImmutabilityType)coronaConfig.readEntry("immutability", static_cast<int>(Plasma::Types::Mutable)));
    }
}

void ShellCorona::primaryOutputChanged()
{
    if (!m_desktopViewforId.contains(0)) {
        return;
    }

    //when the appearance of a new primary screen *moves*
    //the position of the now secondary, the two screens will appear overlapped for an instant, and a spurious output redundant would happen here if checked immediately
    m_reconsiderOutputsTimer.start();

    QScreen *oldPrimary = m_desktopViewforId.value(0)->screen();
    QScreen *newPrimary = qGuiApp->primaryScreen();
    if (!newPrimary || newPrimary == oldPrimary) {
        return;
    }

    qWarning()<<"Old primary output:"<<oldPrimary<<"New primary output:"<<newPrimary;
    const int oldIdOfPrimary = m_screenPool->id(newPrimary->name());
    m_screenPool->setPrimaryConnector(newPrimary->name());
    //swap order in m_desktopViewforId
    if (m_desktopViewforId.contains(0) && m_desktopViewforId.contains(oldIdOfPrimary)) {
        DesktopView *primaryDesktop = m_desktopViewforId.value(0);
        DesktopView *oldDesktopOfPrimary = m_desktopViewforId.value(oldIdOfPrimary);

        primaryDesktop->setScreenToFollow(newPrimary);
        oldDesktopOfPrimary->setScreenToFollow(oldPrimary);
        primaryDesktop->show();
        oldDesktopOfPrimary->show();
    //corner case: the new primary screen was added into redundant outputs when appeared, *and* !m_desktopViewforId.contains(oldIdOfPrimary)
    //meaning that we had only one screen, connected a new oone that
    //a) is now primary and
    //b) is at 0,0 position, moving the current screen out of the way
    // and this will always happen in two events
    } else if (m_desktopViewforId.contains(0) && m_redundantOutputs.contains(newPrimary)) {
        m_desktopViewforId[0]->setScreenToFollow(newPrimary);
        m_redundantOutputs.remove(newPrimary);
        m_redundantOutputs.insert(oldPrimary);
    }

    for (PanelView *panel : qAsConst(m_panelViews)) {
        if (panel->screen() == oldPrimary) {
            panel->setScreenToFollow(newPrimary);
        } else if (panel->screen() == newPrimary) {
            panel->setScreenToFollow(oldPrimary);
        }
    }

    //can't do the screen invariant here as reconsideroutputs wasn't executed yet
    //CHECK_SCREEN_INVARIANTS
}

#ifndef NDEBUG
void ShellCorona::screenInvariants() const
{
    Q_ASSERT(m_desktopViewforId.keys().count() <= QGuiApplication::screens().count());

    QSet<QScreen*> screens;
    foreach (const int id, m_desktopViewforId.keys()) {
        const DesktopView *view = m_desktopViewforId.value(id);
        QScreen *screen = view->screenToFollow();
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
            //If any kscreen related activities occurred
            //during startup, the panel wouldn't be visible yet, and this would assert
            if (panel->containment()->isUiReady()) {
                Q_ASSERT(panel->isVisible());
            }
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
    const QUrl alternativesQML = kPackage().fileUrl("appletalternativesui");
    if (alternativesQML.isEmpty()) {
        return;
    }

    auto *qmlObj = new KDeclarative::QmlObjectSharedEngine(this);
    qmlObj->setInitializationDelayed(true);
    qmlObj->setSource(alternativesQML);

    AlternativesHelper *helper = new AlternativesHelper(applet, qmlObj);
    qmlObj->rootContext()->setContextProperty(QStringLiteral("alternativesHelper"), helper);

    qmlObj->completeInitialization();

    auto dialog = qobject_cast<PlasmaQuick::Dialog*>(qmlObj->rootObject());
    if (!dialog) {
        qWarning() << "Alternatives UI does not inherit from Dialog";
        delete qmlObj;
        return;
    }
    connect(applet, &Plasma::Applet::destroyedChanged, qmlObj, [qmlObj] (bool destroyed) {
        if (!destroyed) {
            return;
        }
        qmlObj->deleteLater();
    });
    connect(dialog, &PlasmaQuick::Dialog::visibleChanged, qmlObj, [qmlObj](bool visible) {
        if (visible) {
            return;
        }
        qmlObj->deleteLater();
    });
}

void ShellCorona::unload()
{
    if (m_shell.isEmpty()) {
        return;
    }
    qDeleteAll(m_desktopViewforId);
    m_desktopViewforId.clear();
    qDeleteAll(m_panelViews);
    m_panelViews.clear();
    m_waitingPanels.clear();
    m_activityContainmentPlugins.clear();

    while (!containments().isEmpty()) {
        // Some applets react to destroyedChanged rather just destroyed,
        // give them  the possibility to react
        // deleting a containment will remove it from the list due to QObject::destroyed connect in Corona
        // this form doesn't crash, while qDeleteAll(containments()) does
        // And is more correct anyways to use destroy()
        containments().first()->destroy();
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
    //pre-startup scripts
    QString script = m_lookAndFeelPackage.filePath("layouts", QString(shell() + "-prelayout.js").toLatin1());
    if (!script.isEmpty()) {
          QFile file(script);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text) ) {
                QString code = file.readAll();
                qDebug() << "evaluating pre-startup script:" << script;

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
    }

    //NOTE: Is important the containments already exist for each screen
    // at the moment of the script execution,the same loop in :load()
    // is executed too late
    for (QScreen* screen : qGuiApp->screens()) {
        addOutput(screen);
    }

    script = m_testModeLayout;

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
        const QStringList existingActivities = m_activityController->activities();
        for (const QString &id : existingActivities) {
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
    const QStringList scripts = WorkspaceScripting::ScriptEngine::pendingUpdateScripts(this);
    if (scripts.isEmpty()) {
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

    for (const QString &script : scripts) {
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
    DesktopView* view = m_desktopViewforId.value(id);
    if (!view) {
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = qGuiApp->primaryScreen();
        return s ? s->geometry() : QRect();
    }
    return view->geometry();
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    return m_strutManager->availableScreenRegion(id);
}

QRegion ShellCorona::_availableScreenRegion(int id) const
{
    DesktopView* view = m_desktopViewforId.value(id);
    if (!view) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = qGuiApp->primaryScreen();
        return s ? s->availableGeometry() : QRegion();
    }

    QRegion r = view->geometry();
    for (const PanelView *v : m_panelViews) {
        if (v->isVisible() && view->screen() == v->screen() && v->visibilityMode() != PanelView::AutoHide) {
            //if the panel is being moved around, we still want to calculate it from the edge
            r -= v->geometryByDistance(0);
        }
    }
    return r;
}

QRect ShellCorona::availableScreenRect(int id) const
{
    return m_strutManager->availableScreenRect(id);
}

QRect ShellCorona::_availableScreenRect(int id) const
{
    DesktopView *view = m_desktopViewforId.value(id);
    if (!view) {
        //each screen should have a view
        qWarning() << "requesting unexisting screen" << id;
        QScreen *s = qGuiApp->primaryScreen();
        return s ? s->availableGeometry() : QRect();
    }

    QRect r = view->geometry();
    for (PanelView *v : m_panelViews) {
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

    auto itDesktop = m_desktopViewforId.find(idx);
    if (itDesktop == m_desktopViewforId.end()) {
        return;
    }
    Q_ASSERT(m_desktopViewforId.value(idx) == desktopView);

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

    m_desktopViewforId.erase(itDesktop);
    delete desktopView;

    emit screenRemoved(idx);
}

PanelView *ShellCorona::panelView(Plasma::Containment *containment) const
{
    return m_panelViews.value(containment);
}

///// SLOTS

QList<PanelView *> ShellCorona::panelsForScreen(QScreen *screen) const
{
    QList<PanelView *> ret;
    for (PanelView *v : m_panelViews) {
        if (v->screenToFollow() == screen) {
            ret += v;
        }
    }
    return ret;
}

DesktopView* ShellCorona::desktopForScreen(QScreen* screen) const
{
    return m_desktopViewforId.value(m_screenPool->id(screen->name()));
}

void ShellCorona::handleScreenRemoved(QScreen* screen)
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
    const QRect thisGeometry = screen->geometry();

    const int thisId = m_screenPool->id(screen->name());

    //FIXME: QScreen doesn't have any idea of "this qscreen is clone of this other one
    //so this ultra inefficient heuristic has to stay until we have a slightly better api
    //logic is:
    //a screen is redundant if:
    //* its geometry is contained in another one
    //* if their resolutions are different, the "biggest" one wins
    //* if they have the same geometry, the one with the lowest id wins (arbitrary, but gives reproducible behavior and makes the primary screen win)
    const auto screens = qGuiApp->screens();
    for (QScreen* s : screens) {
        //don't compare with itself
        if (screen == s) {
            continue;
        }

        const QRect otherGeometry = s->geometry();

        const int otherId = m_screenPool->id(s->name());

        if (otherGeometry.contains(thisGeometry, false) &&
            (//since at this point contains is true, if either
             //measure of othergeometry is bigger, has a bigger area
             otherGeometry.width() > thisGeometry.width() ||
             otherGeometry.height() > thisGeometry.height() ||
             //ids not -1 are considered in descending order of importance
             //-1 means that is a screen not known yet, just arrived and
             //not yet in screenpool: this happens for screens that
             //are hotplugged and weren't known. it does NOT happen
             //at first startup, as screenpool populates on load with all screens connected at the moment before the rest of the shell starts up
             (thisId == -1 && otherId != -1) ||
             (thisId > otherId && otherId != -1))) {
            return true;
        }
    }

    return false;
}

void ShellCorona::reconsiderOutputs()
{
    const auto screens = qGuiApp->screens();
    for (QScreen* screen : screens) {
        if (m_redundantOutputs.contains(screen)) {
            if (!isOutputRedundant(screen)) {
                //qDebug() << "not redundant anymore" << screen;
                addOutput(screen);
            }
        } else if (isOutputRedundant(screen)) {
            qDebug() << "new redundant screen" << screen << "with primary screen" << qGuiApp->primaryScreen();

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

    if (view->rendererInterface()->graphicsApi() != QSGRendererInterface::Software) {
        connect(view, &QQuickWindow::sceneGraphError, this, &ShellCorona::glInitializationFailed);
    }
    connect(view, &DesktopView::geometryChanged, this, [=]() {
        const int id = m_screenPool->id(view->screen()->name());
        if (id >= 0) {
            emit screenGeometryChanged(id);
            emit availableScreenRegionChanged();
            emit availableScreenRectChanged();
        }
    });

    Plasma::Containment *containment = createContainmentForActivity(m_activityController->currentActivity(), insertPosition);
    Q_ASSERT(containment);

    QAction *removeAction = containment->actions()->action(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }

    connect(containment, &Plasma::Containment::uiReadyChanged, this, &ShellCorona::checkAllDesktopsUiReady);

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
    emit screenAdded(m_screenPool->id(screen->name()));

    CHECK_SCREEN_INVARIANTS
}

void ShellCorona::checkAllDesktopsUiReady(bool ready)
{
    if (!ready)
        return;
    for (auto v : qAsConst(m_desktopViewforId)) {
        if (!v->containment()->isUiReady())
            return;

        qDebug() << "Plasma Shell startup completed";
        QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                        QStringLiteral("/KSplash"),
                                        QStringLiteral("org.kde.KSplash"),
                                        QStringLiteral("setStage"));
        ksplashProgressMessage.setArguments(QList<QVariant>() << QStringLiteral("desktop"));
        QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
    }
}

Plasma::Containment *ShellCorona::createContainmentForActivity(const QString& activity, int screenNum)
{
    for (Plasma::Containment *cont : containmentsForActivity(activity)) {
        //in the case of a corrupt config file
        //with multiple containments with same lastScreen
        //it can happen two insertContainment happen for
        //the same screen, leading to the old containment
        //to be destroyed
        if (!cont->destroyed() && cont->screen() == screenNum) {
            return cont;
        }
    }

    QString plugin = m_activityContainmentPlugins.value(activity);

    if (plugin.isEmpty()) {
        plugin = defaultContainmentPlugin();
    }

    Plasma::Containment *containment = containmentForScreen(screenNum, activity, plugin, QVariantList());
    Q_ASSERT(containment);

    if (containment) {
        insertContainment(activity, screenNum, containment);
    }

    return containment;
}

void ShellCorona::createWaitingPanels()
{
    QList<Plasma::Containment *> stillWaitingPanels;

    for (Plasma::Containment *cont : qAsConst(m_waitingPanels)) {
        //ignore non existing (yet?) screens
        int requestedScreen = cont->lastScreen();
        if (requestedScreen < 0) {
            requestedScreen = 0;
        }

        DesktopView* desktopView = m_desktopViewforId.value(requestedScreen);
        if (!desktopView) {
            stillWaitingPanels << cont;
            continue;
        }

        //TODO: does a similar check make sense?
        //Q_ASSERT(qBound(0, requestedScreen, m_screenPool->count() - 1) == requestedScreen);
        QScreen *screen = desktopView->screenToFollow();
        PanelView* panel = new PanelView(this, screen);
        if (panel->rendererInterface()->graphicsApi() != QSGRendererInterface::Software) {
            connect(panel, &QQuickWindow::sceneGraphError, this, &ShellCorona::glInitializationFailed);
        }
        connect(panel, &QWindow::visibleChanged, this, &Plasma::Corona::availableScreenRectChanged);
        connect(panel, &QWindow::screenChanged, this, &Plasma::Corona::availableScreenRectChanged);
        connect(panel, &PanelView::locationChanged, this, &Plasma::Corona::availableScreenRectChanged);
        connect(panel, &PanelView::visibilityModeChanged, this, &Plasma::Corona::availableScreenRectChanged);
        connect(panel, &PanelView::thicknessChanged, this, &Plasma::Corona::availableScreenRectChanged);

        m_panelViews[cont] = panel;
        panel->setContainment(cont);
        cont->reactToScreenChange();

        connect(cont, &QObject::destroyed, this, &ShellCorona::panelContainmentDestroyed);
    }
    m_waitingPanels = stillWaitingPanels;
    emit availableScreenRectChanged();
}

void ShellCorona::panelContainmentDestroyed(QObject *cont)
{
    auto view = m_panelViews.take(static_cast<Plasma::Containment*>(cont));
    delete view;
    //don't make things relayout when the application is quitting
    //NOTE: qApp->closingDown() is still false here
    if (!m_closingDown) {
        emit availableScreenRectChanged();
    }
}

void ShellCorona::handleContainmentAdded(Plasma::Containment *c)
{
    connect(c, &Plasma::Containment::showAddWidgetsInterface,
            this, &ShellCorona::toggleWidgetExplorer);
    connect(c, &Plasma::Containment::appletAlternativesRequested,
            this, &ShellCorona::showAlternativesForApplet);
    connect(c, &Plasma::Containment::appletCreated, this, [this, c] (Plasma::Applet *applet) {
        executeSetupPlasmoidScript(c, applet);
    });
}

void ShellCorona::executeSetupPlasmoidScript(Plasma::Containment *containment, Plasma::Applet *applet)
{
    if (!applet->pluginMetaData().isValid() || !containment->pluginMetaData().isValid()) {
        return;
    }

    const QString scriptFile = m_lookAndFeelPackage.filePath("plasmoidsetupscripts", applet->pluginMetaData().pluginId() + ".js");

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
    for (DesktopView *view : qAsConst(m_desktopViewforId)) {
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
    for (DesktopView *view : qAsConst(m_desktopViewforId)) {
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
        m_interactiveConsole = nullptr;
        return;
    }

    if (!m_interactiveConsole) {
        const QUrl consoleQML = kPackage().fileUrl("interactiveconsole");
        if (consoleQML.isEmpty()) {
            return;
        }

        m_interactiveConsole = new KDeclarative::QmlObjectSharedEngine(this);
        m_interactiveConsole->setInitializationDelayed(true);
        m_interactiveConsole->setSource(consoleQML);

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
    if (calledFromDBus()) {
        if (immutability() == Plasma::Types::SystemImmutable) {
            sendErrorReply(QDBusError::Failed, QStringLiteral("Widgets are locked"));
            return;
        } else if (!KAuthorized::authorize(QStringLiteral("plasma-desktop/scripting_console"))) {
            sendErrorReply(QDBusError::Failed, QStringLiteral("Administrative policies prevent script execution"));
            return;
        }
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
    if (!scriptEngine.errorString().isEmpty() && calledFromDBus()) {
        sendErrorReply(QDBusError::Failed, scriptEngine.errorString());
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

    const QStringList existingActivities = m_activityController->activities();
    for (const QString &id : existingActivities) {
        activityAdded(id);
    }

    // Checking whether the result we got is valid. Just in case.
    Q_ASSERT_X(!existingActivities.isEmpty(), "isEmpty", "There are no activities, and the service is running");
    Q_ASSERT_X(existingActivities[0] != QLatin1String("00000000-0000-0000-0000-000000000000"),
            "null uuid", "There is a nulluuid activity present");

    // Killing the unassigned containments
    const auto conts = containments();
    for (Plasma::Containment *cont : conts) {
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

    for (auto it = m_desktopViewforId.constBegin(); it != m_desktopViewforId.constEnd(); ++it) {
        Plasma::Containment *c = createContainmentForActivity(newActivity, it.key());

        QAction *removeAction = c->actions()->action(QStringLiteral("remove"));
        if (removeAction) {
            removeAction->deleteLater();
        }
        (*it)->setContainment(c);
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
    for (auto cont : containmentsForActivity(id)) {
        cont->destroy();
    }
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
    for (auto it = m_desktopViewforId.constBegin(); it != m_desktopViewforId.constEnd(); ++it) {
        Plasma::Containment *c = createContainmentForActivity(id, it.key());
        if (c) {
            c->config().writeEntry("lastScreen", it.key());
        }
    }
}

Plasma::Containment *ShellCorona::setContainmentTypeForScreen(int screen, const QString &plugin)
{
    //search but not create
    Plasma::Containment *oldContainment = containmentForScreen(screen, m_activityController->currentActivity(), QString());

    //no valid containment in given screen, giving up
    if (!oldContainment) {
        return nullptr;
    }

    if (plugin.isEmpty()) {
        return oldContainment;
    }

    DesktopView *view = nullptr;
    for (DesktopView *v : qAsConst(m_desktopViewforId)) {
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
    } else if (!newContainment->pluginMetaData().isValid()) {
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

    const auto groups = oldCg.groupList();
    for (const QString &group : groups) {
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
    newContainment->flushPendingConstraintsEvents();
    emit containmentAdded(newContainment);

    //Move the applets
    const auto applets = oldContainment->applets();
    for (Plasma::Applet *applet : applets) {
        newContainment->addApplet(applet);
    }

    //remove the "remove" action
    QAction *removeAction = newContainment->actions()->action(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }
    view->setContainment(newContainment);
    newContainment->setActivity(oldContainment->activity());
    insertContainment(oldContainment->activity(), screen, newContainment);

    //removing the focus from the item that is going to be destroyed
    //fixes a crash
    //delayout the destruction of the old containment fixes another crash
    view->rootObject()->setFocus(true, Qt::MouseFocusReason);
    QTimer::singleShot(2500, oldContainment, &Plasma::Applet::destroy);

    //Save now as we now have a screen, so lastScreen will not be -1
    newContainment->save(newCg);
    requestConfigSync();
    emit availableScreenRectChanged();

    return newContainment;
}

void ShellCorona::checkAddPanelAction(const QStringList &sycocaChanges)
{
    if (!sycocaChanges.isEmpty() && !sycocaChanges.contains(QLatin1String("services"))) {
        return;
    }

    delete m_addPanelAction;
    m_addPanelAction = nullptr;

    m_addPanelsMenu.reset(nullptr);

    KPluginInfo::List panelContainmentPlugins = Plasma::PluginLoader::listContainmentsOfType(QStringLiteral("Panel"));

    auto filter = [](const KPluginMetaData &md) -> bool
    {
        return md.value(QStringLiteral("NoDisplay")) != QLatin1String("true") && KPluginMetaData::readStringList(md.rawData(), QStringLiteral("X-Plasma-ContainmentCategories")).contains(QLatin1String("panel"));
    };
    QList<KPluginMetaData> templates = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), filter);

    if (panelContainmentPlugins.count() + templates.count() == 1) {
        m_addPanelAction = new QAction(i18n("Add Panel"), this);
        m_addPanelAction->setData(Plasma::Types::AddAction);
        connect(m_addPanelAction, SIGNAL(triggered(bool)), this, SLOT(addPanel()));
    } else if (!panelContainmentPlugins.isEmpty()) {
        m_addPanelsMenu.reset(new QMenu);
        m_addPanelAction = m_addPanelsMenu->menuAction();
        m_addPanelAction->setText(i18n("Add Panel"));
        m_addPanelAction->setData(Plasma::Types::AddAction);
        connect(m_addPanelsMenu.data(), &QMenu::aboutToShow, this, &ShellCorona::populateAddPanelsMenu);
        connect(m_addPanelsMenu.data(), SIGNAL(triggered(QAction*)), this, SLOT(addPanel(QAction*)));
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

    const KPluginInfo::List panelContainmentPlugins = Plasma::PluginLoader::listContainmentsOfType(QStringLiteral("Panel"));
    QMap<QString, QPair<KPluginInfo, KPluginMetaData> > sorted;
    for (const KPluginInfo &plugin : panelContainmentPlugins) {
        if (plugin.property(QStringLiteral("NoDisplay")).toString() == QLatin1String("true")) {
            continue;
        }
        sorted.insert(plugin.name(), qMakePair(plugin, KPluginMetaData()));
    }

    auto filter = [](const KPluginMetaData &md) -> bool
    {
        return md.value(QStringLiteral("NoDisplay")) != QLatin1String("true") && KPluginMetaData::readStringList(md.rawData(), QStringLiteral("X-Plasma-ContainmentCategories")).contains(QLatin1String("panel"));
    };
    const QList<KPluginMetaData> templates = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), filter);
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
        return nullptr;
    }

    QList<Plasma::Types::Location> availableLocations;
    availableLocations << Plasma::Types::LeftEdge << Plasma::Types::TopEdge << Plasma::Types::RightEdge << Plasma::Types::BottomEdge;

    for (auto it = m_panelViews.constBegin(); it != m_panelViews.constEnd(); ++it) {
        availableLocations.removeAll((*it)->location());
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
    //not creating the panel view yet in order to have the same code path
    //between the first and subsequent plasma starts. we want to have the panel appearing only when its layout is completed, to not have
    //many visible relayouts. otherwise we had even panel resizes at startup that
    //made al lthe full representations be loaded.
    m_waitingPanelsTimer.start();

    const QPoint cursorPos(QCursor::pos());
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        //m_panelViews.contains(panel) == false iff addPanel is executed in a startup script
        auto panelView = m_panelViews.value(panel);
        if (panelView && screen->geometry().contains(cursorPos)) {
            panelView->setScreenToFollow(screen);
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
    for (auto it = m_desktopViewforId.constBegin(), end = m_desktopViewforId.constEnd(); it != end; ++it) {
        if (it.value()->containment() == containment && containment->activity() == m_activityController->currentActivity()) {
            return it.key();
        }
    }

    //if the panel views already exist, base upon them
    PanelView *view = m_panelViews.value(containment);
    if (view && view->screenToFollow()) {
        return m_screenPool->id(view->screenToFollow()->name());
    }

    //Failed? fallback on lastScreen()
    //lastScreen() is the correct screen for panels
    //It is also correct for desktops *that have the correct activity()*
    //a containment with lastScreen() == 0 but another activity,
    //won't be associated to a screen
//     qDebug() << "ShellCorona screenForContainment: " << containment << " Last screen is " << containment->lastScreen();

    for (auto screen : qGuiApp->screens()) {
        // containment->lastScreen() == m_screenPool->id(screen->name()) to check if the lastScreen refers to a screen that exists/it's known
        if (containment->lastScreen() == m_screenPool->id(screen->name()) &&
            (containment->activity() == m_activityController->currentActivity() ||
            containment->containmentType() == Plasma::Types::PanelContainment || containment->containmentType() == Plasma::Types::CustomPanelContainment)) {
            return containment->lastScreen();
        }
    }

    return -1;
}

void ShellCorona::nextActivity()
{
    m_activityController->nextActivity();
}

void ShellCorona::previousActivity()
{
    m_activityController->previousActivity();
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
    Plasma::Containment *cont = nullptr;
    auto candidates = containmentsForActivity(activity);
    for (Plasma::Containment *c : candidates) {
        //using lastScreen() instead of screen() catches also containments of activities that aren't the current one, so not assigned to a screen right now
        if (c->lastScreen() == screenNum) {
            cont = c;
            if (containment == cont) {
                return;
            }
            break;
        }
    }

    Q_ASSERT(containment != cont);

    //if there was a duplicate containment destroy the old one
    //the new one replaces it
    //FIXME: this whole function is probably redundant now
    if (cont) {
        cont->destroy();
    }
}

/**
 * @internal
 *
 * The DismissPopupEventFilter class monitors mouse button press events and
 * when needed dismisses the active popup widget.
 *
 * plasmashell uses both QtQuick and QtWidgets under a single roof, QtQuick is
 * used for most of things, while QtWidgets is used for things such as context
 * menus, etc.
 *
 * If user clicks outside a popup window, it's expected that the popup window
 * will be closed.  On X11, it's achieved by establishing both a keyboard grab
 * and a pointer grab. But on Wayland, you can't grab keyboard or pointer. If
 * user clicks a surface of another app, the compositor will dismiss the popup
 * surface.  However, if user clicks some surface of the same application, the
 * popup surface won't be dismissed, it's up to the application to decide
 * whether the popup must be closed. In 99% cases, it must.
 *
 * Qt has some code that dismisses the active popup widget if another window
 * of the same app has been clicked. But, that code works only if the
 * application uses solely Qt widgets. See QTBUG-83972. For plasma it doesn't
 * work, because as we said previously, it uses both Qt Quick and Qt Widgets.
 *
 * Ideally, this bug needs to be fixed upstream, but given that it'll involve
 * major changes in Qt, the chances of it being fixed any time soon are slim.
 *
 * In order to work around the popup dismissal bug, we install an event filter
 * that monitors Qt::MouseButtonPress events. If it happens that user has
 * clicked outside an active popup widget, that popup will be closed. This
 * event filter is not needed on X11!
 */
class DismissPopupEventFilter : public QObject
{
    Q_OBJECT

public:
    explicit DismissPopupEventFilter(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool m_filterMouseEvents = false;
};

DismissPopupEventFilter::DismissPopupEventFilter(QObject *parent)
    : QObject(parent)
{
}

bool DismissPopupEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        if (m_filterMouseEvents) {
            // Eat events until all mouse buttons are released.
            return true;
        }

        QWidget *popup = QApplication::activePopupWidget();
        if (!popup) {
            return false;
        }

        QWindow *window = qobject_cast<QWindow *>(watched);
        if (popup->windowHandle() == window) {
            // The popup window handles mouse events before the widget.
            return false;
        }

        QWidget *widget = qobject_cast<QWidget *>(watched);
        if (widget) {
            // Let the popup widget handle the mouse press event.
            return false;
        }

        popup->close();
        m_filterMouseEvents = true;
        return true;

    } else if (event->type() == QEvent::MouseButtonRelease) {
        if (m_filterMouseEvents) {
            // Eat events until all mouse buttons are released.
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->buttons() == Qt::NoButton) {
                m_filterMouseEvents = false;
            }
            return true;
        }
    }

    return false;
}

void ShellCorona::setupWaylandIntegration()
{
    if (!KWindowSystem::isPlatformWayland()) {
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
    connection->roundtrip();
    qApp->installEventFilter(new DismissPopupEventFilter(this));
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
    for (PanelView* view : qAsConst(m_panelViews)) {
        view->updateStruts();
    }
}

void ShellCorona::configurationChanged(const QString &path)
{
    if (path == m_configPath) {
        m_config->reparseConfiguration();
    }
}

void ShellCorona::activateLauncherMenu()
{
    for (auto it = m_panelViews.constBegin(), end = m_panelViews.constEnd(); it != end; ++it) {
        const auto applets = it.key()->applets();
        for (auto applet : applets) {
            const auto provides = KPluginMetaData::readStringList(applet->pluginMetaData().rawData(), QStringLiteral("X-Plasma-Provides"));
            if (provides.contains(QLatin1String("org.kde.plasma.launchermenu"))) {
                if (!applet->globalShortcut().isEmpty()) {
                    emit applet->activated();
                    return;
                }
            }
        }
    }
}

void ShellCorona::activateTaskManagerEntry(int index)
{
    auto activateTaskManagerEntryOnContainment = [](const Plasma::Containment *c, int index) {
        const auto &applets = c->applets();
        for (auto *applet : applets) {
            const auto &provides = KPluginMetaData::readStringList(applet->pluginMetaData().rawData(), QStringLiteral("X-Plasma-Provides"));
            if (provides.contains(QLatin1String("org.kde.plasma.multitasking"))) {
                if (QQuickItem *appletInterface = applet->property("_plasma_graphicObject").value<QQuickItem *>()) {
                    const auto &childItems = appletInterface->childItems();
                    if (childItems.isEmpty()) {
                        continue;
                    }

                    for (QQuickItem *item : childItems) {
                        if (auto *metaObject = item->metaObject()) {
                            // not using QMetaObject::invokeMethod to avoid warnings when calling
                            // this on applets that don't have it or other child items since this
                            // is pretty much trial and error.

                            // Also, "var" arguments are treated as QVariant in QMetaObject
                            int methodIndex = metaObject->indexOfMethod("activateTaskAtIndex(QVariant)");
                            if (methodIndex == -1) {
                                continue;
                            }

                            QMetaMethod method = metaObject->method(methodIndex);
                            if (method.invoke(item, Q_ARG(QVariant, index))) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    };

    // To avoid overly complex configuration, we'll try to get the 90% usecase to work
    // which is activating a task on the task manager on a panel on the primary screen.

    for (auto it = m_panelViews.constBegin(), end = m_panelViews.constEnd(); it != end; ++it) {
        if (it.value()->screen() != qGuiApp->primaryScreen()) {
            continue;
        }
        if (activateTaskManagerEntryOnContainment(it.key(), index)) {
            return;
        }
    }

    // we didn't find anything on primary, try all the panels
    for (auto it = m_panelViews.constBegin(), end = m_panelViews.constEnd(); it != end; ++it) {
        if (activateTaskManagerEntryOnContainment(it.key(), index)) {
            return;
        }
    }
}

// Desktop corona handler


#include "moc_shellcorona.cpp"
#include "shellcorona.moc"

