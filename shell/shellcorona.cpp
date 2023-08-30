/*
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2013 Ivan Cukic <ivan.cukic@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "shellcorona.h"
#include "debug.h"
#include "strutmanager.h"

#include <config-plasma.h>

#include <QApplication>
#include <QDBusConnection>
#include <QDebug>
#include <QMenu>
#include <QQmlContext>
#include <QQuickItemGrabResult>
#include <QScreen>
#include <QUrl>

#include <QJsonDocument>
#include <QJsonObject>

#include <Plasma/PluginLoader>
#include <PlasmaQuick/AppletQuickItem>
#include <PlasmaQuick/Dialog>
#include <kactioncollection.h>
#include <klocalizedstring.h>

#include <KAuthorized>
#include <KGlobalAccel>
#include <KMessageBox>
#include <KWindowSystem>
#include <KX11Extras>
#include <PlasmaQuick/SharedQmlEngine>
#include <kactivities/consumer.h>
#include <kactivities/controller.h>
#include <kdirwatch.h>
#include <ksycoca.h>

#include <KPackage/PackageLoader>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/plasmashell.h>
#include <KWayland/Client/plasmawindowmanagement.h>
#include <KWayland/Client/registry.h>
#include <plasma/plasma.h>

#include "config-ktexteditor.h" // HAVE_KTEXTEDITOR

#include "alternativeshelper.h"
#include "desktopview.h"
#include "osd.h"
#include "panelview.h"
#include "screenpool.h"
#if USE_SCRIPTING
#include "scripting/scriptengine.h"
#endif
#include "shellcontainmentconfig.h"

#include "debug.h"
#include "futureutil.h"
#include "plasmashelladaptor.h"

#ifndef NDEBUG
#define CHECK_SCREEN_INVARIANTS screenInvariants();
#else
#define CHECK_SCREEN_INVARIANTS
#endif

#if HAVE_X11
#include <NETWM>
#include <private/qtx11extras_p.h>
#include <xcb/xcb.h>
#endif
#include <chrono>

using namespace std::chrono_literals;
static const int s_configSyncDelay = 10000; // 10 seconds

ShellCorona::ShellCorona(QObject *parent)
    : Plasma::Corona(parent)
    , m_config(KSharedConfig::openConfig(QStringLiteral("plasmarc")))
    , m_screenPool(new ScreenPool(this))
    , m_activityController(new KActivities::Controller(this))
    , m_addPanelAction(nullptr)
    , m_addPanelsMenu(nullptr)
    , m_waylandPlasmaShell(nullptr)
    , m_closingDown(false)
    , m_strutManager(new StrutManager(this))
    , m_shellContainmentConfig(nullptr)
{
    setupWaylandIntegration();
    qmlRegisterUncreatableType<DesktopView>("org.kde.plasma.shell", 2, 0, "Desktop", QStringLiteral("It is not possible to create objects of type Desktop"));
    qmlRegisterUncreatableType<PanelView>("org.kde.plasma.shell", 2, 0, "Panel", QStringLiteral("It is not possible to create objects of type Panel"));

    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    m_lookAndFeelPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), packageName);

    // Accent color setting
    KSharedConfigPtr globalConfig = KSharedConfig::openConfig();
    KConfigGroup accentColorConfigGroup(globalConfig, "General");
    m_accentColorFromWallpaperEnabled = accentColorConfigGroup.readEntry("accentColorFromWallpaper", false);

    m_accentColorConfigWatcher = KConfigWatcher::create(globalConfig);
    connect(m_accentColorConfigWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (names.contains(QByteArrayLiteral("accentColorFromWallpaper"))) {
            const bool result = group.readEntry("accentColorFromWallpaper", false);
            if (m_accentColorFromWallpaperEnabled != result) {
                m_accentColorFromWallpaperEnabled = result;
                Q_EMIT accentColorFromWallpaperEnabledChanged();
            }
        }
    });
}

void ShellCorona::init()
{
#if USE_SCRIPTING
    connect(this, &Plasma::Corona::containmentCreated, this, [this](Plasma::Containment *c) {
        executeSetupPlasmoidScript(c, c);
    });
#endif

    connect(this, &Plasma::Corona::availableScreenRectChanged, this, &Plasma::Corona::availableScreenRegionChanged);

    m_appConfigSyncTimer.setSingleShot(true);
    m_appConfigSyncTimer.setInterval(s_configSyncDelay);
    connect(&m_appConfigSyncTimer, &QTimer::timeout, this, &ShellCorona::syncAppConfig);
    // we want our application config with screen mapping to always be in sync with the applets one, so a crash at any time will still
    // leave containments pointing to the correct screens
    connect(this, &Corona::configSynced, this, &ShellCorona::syncAppConfig);

    m_waitingPanelsTimer.setSingleShot(true);
    m_waitingPanelsTimer.setInterval(250ms);
    connect(&m_waitingPanelsTimer, &QTimer::timeout, this, &ShellCorona::createWaitingPanels);

#ifndef NDEBUG
    m_invariantsTimer.setSingleShot(true);
    m_invariantsTimer.setInterval(250ms);
    connect(&m_invariantsTimer, &QTimer::timeout, this, &ShellCorona::screenInvariants);
#endif

    m_desktopDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(kPackage().filePath("defaults")), "Desktop");
    m_lnfDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(m_lookAndFeelPackage.filePath("defaults")), "Desktop");
    m_lnfDefaultsConfig = KConfigGroup(&m_lnfDefaultsConfig, QStringLiteral("org.kde.plasma.desktop"));

    new PlasmaShellAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/PlasmaShell"), this);

    // Look for theme config in plasmarc, if it isn't configured, take the theme from the
    // LookAndFeel package, if either is set, change the default theme

    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        // saveLayout is a slot but arguments not compatible
        m_closingDown = true;
        saveLayout();
    });

    connect(this, &ShellCorona::containmentAdded, this, &ShellCorona::handleContainmentAdded);

    QAction *dashboardAction = new QAction(this);
    setAction(QStringLiteral("show dashboard"), dashboardAction);
    QObject::connect(dashboardAction, &QAction::triggered, this, &ShellCorona::setDashboardShown);
    dashboardAction->setText(i18n("Show Desktop"));
    connect(KWindowSystem::self(), &KWindowSystem::showingDesktopChanged, dashboardAction, [dashboardAction](bool showing) {
        dashboardAction->setText(showing ? i18n("Hide Desktop") : i18n("Show Desktop"));
        dashboardAction->setChecked(showing);
    });

    dashboardAction->setAutoRepeat(true);
    dashboardAction->setCheckable(true);
    dashboardAction->setIcon(QIcon::fromTheme(QStringLiteral("dashboard-show")));
    KGlobalAccel::self()->setGlobalShortcut(dashboardAction, Qt::CTRL | Qt::Key_F12);

    checkAddPanelAction();
    connect(KSycoca::self(), &KSycoca::databaseChanged, this, &ShellCorona::checkAddPanelAction);

    // Activity stuff
    QAction *activityAction = new QAction(this);
    setAction(QStringLiteral("manage activities"), activityAction);
    connect(activityAction, &QAction::triggered, this, &ShellCorona::toggleActivityManager);
    activityAction->setText(i18n("Show Activity Switcher"));
    activityAction->setIcon(QIcon::fromTheme(QStringLiteral("activities")));
    activityAction->setShortcut(QKeySequence(QStringLiteral("alt+d, alt+a")));
    activityAction->setShortcutContext(Qt::ApplicationShortcut);

    KGlobalAccel::self()->setGlobalShortcut(activityAction, Qt::META | Qt::Key_Q);

    QAction *stopActivityAction = new QAction(this);
    setAction(QStringLiteral("stop current activity"), stopActivityAction);
    QObject::connect(stopActivityAction, &QAction::triggered, this, &ShellCorona::stopCurrentActivity);

    stopActivityAction->setText(i18n("Stop Current Activity"));
    stopActivityAction->setVisible(false);

    KGlobalAccel::self()->setGlobalShortcut(stopActivityAction, Qt::META | Qt::Key_S);

    QAction *previousActivityAction = new QAction(this);
    setAction(QStringLiteral("switch to previous activity"), previousActivityAction);
    connect(previousActivityAction, &QAction::triggered, this, &ShellCorona::previousActivity);
    previousActivityAction->setText(i18n("Switch to Previous Activity"));
    previousActivityAction->setShortcutContext(Qt::ApplicationShortcut);

    KGlobalAccel::self()->setGlobalShortcut(previousActivityAction, QKeySequence());

    QAction *nextActivityAction = new QAction(this);
    setAction(QStringLiteral("switch to next activity"), nextActivityAction);
    connect(nextActivityAction, &QAction::triggered, this, &ShellCorona::nextActivity);
    nextActivityAction->setText(i18n("Switch to Next Activity"));
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

    connect(qApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *focusWindow) {
        if (!focusWindow) {
            setEditMode(false);
        }
    });
    connect(this, &ShellCorona::editModeChanged, this, [this](bool edit) {
        setDashboardShown(edit);
    });

    QAction *manageContainmentsAction = new QAction(this);
    setAction(QStringLiteral("manage-containments"), manageContainmentsAction);
    manageContainmentsAction->setIcon(QIcon::fromTheme(QStringLiteral("preferences-system-windows-effect-fadedesktop")));
    manageContainmentsAction->setText(i18n("Manage Desktops And Panels..."));
    connect(manageContainmentsAction, &QAction::triggered, this, [this]() {
        if (m_shellContainmentConfig == nullptr) {
            m_shellContainmentConfig = new ShellContainmentConfig(this);
            m_shellContainmentConfig->init();
        }
        // Swapping desktop views around causes problems with the show desktop effect
        setEditMode(false);
    });
    auto updateManageContainmentsVisiblility = [this, manageContainmentsAction]() {
        QSet<int> allScreenIds;
        for (auto *cont : containments()) {
            allScreenIds.insert(cont->lastScreen());
        }
        manageContainmentsAction->setVisible(allScreenIds.count() > 1);
    };
    connect(this, &ShellCorona::containmentAdded, this, updateManageContainmentsVisiblility);
    connect(this, &ShellCorona::screenRemoved, this, updateManageContainmentsVisiblility);
    updateManageContainmentsVisiblility();

    QAction *cyclePanelFocusAction = new QAction(this);
    setAction(QStringLiteral("cycle-panels"), cyclePanelFocusAction);
    cyclePanelFocusAction->setText(i18n("Move keyboard focus between panels"));
    KGlobalAccel::self()->setGlobalShortcut(cyclePanelFocusAction, Qt::META | Qt::ALT | Qt::Key_P);
    connect(cyclePanelFocusAction, &QAction::triggered, this, &ShellCorona::slotCyclePanelFocus);

    unload();
    /*
     * we want to make an initial load once we have loaded the activities _IF_ KAMD is running
     * it is valid for KAMD to not be running.
     *
     * Potentially 2 async jobs
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

ShellCorona::~ShellCorona()
{
    while (!containments().isEmpty()) {
        // Deleting a containment will remove it from the list due to QObject::destroyed connect in Corona
        // Deleting a containment in turn also kills any panel views
        delete containments().constFirst();
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
        themeName = shellCfg.readEntry(themeNameKey, "default");
    }

    if (!themeName.isEmpty()) {
        Plasma::Theme *t = new Plasma::Theme(this);
        t->setThemeName(themeName);
    }
}

QJsonObject dumpconfigGroupJS(const KConfigGroup &rootGroup)
{
    QJsonObject result;

    QStringList hierarchy;
    QStringList escapedHierarchy;
    QList<KConfigGroup> groups{rootGroup};
    QSet<QString> visitedNodes;

    const QSet<QString> forbiddenKeys{QStringLiteral("activityId"),
                                      QStringLiteral("ItemsGeometries"),
                                      QStringLiteral("AppletOrder"),
                                      QStringLiteral("SystrayContainmentId"),
                                      QStringLiteral("location"),
                                      QStringLiteral("plugin")};

    auto groupID = [&escapedHierarchy]() {
        return '/' + escapedHierarchy.join('/');
    };

    // Perform a depth-first tree traversal for config groups
    while (!groups.isEmpty()) {
        KConfigGroup cg = groups.last();

        KConfigGroup parentCg = cg;
        // FIXME: name is not enough

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
            // TODO: this is conditional if applet or containment

            const auto map = cg.entryMap();
            auto i = map.cbegin();
            for (; i != map.cend(); ++i) {
                // some blacklisted keys we don't want to save
                if (!forbiddenKeys.contains(i.key())) {
                    configGroupJson.insert(i.key(), i.value());
                }
            }
        }

        const auto groupList = cg.groupList();
        for (const QString &groupName : groupList) {
            if (groupName == QLatin1String("Applets") || visitedNodes.contains(groupID() + '/' + groupName)) {
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

    // same gridUnit calculation as ScriptEngine
    int gridUnit = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();
    if (gridUnit % 2 != 0) {
        gridUnit++;
    }

    auto isPanel = [](Plasma::Containment *cont) {
        return (cont->formFactor() == Plasma::Types::Horizontal || cont->formFactor() == Plasma::Types::Vertical)
            && (cont->location() == Plasma::Types::TopEdge || cont->location() == Plasma::Types::BottomEdge || cont->location() == Plasma::Types::LeftEdge
                || cont->location() == Plasma::Types::RightEdge)
            && cont->pluginMetaData().pluginId() != QLatin1String("org.kde.plasma.private.systemtray");
    };

    auto isDesktop = [](Plasma::Containment *cont) {
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
                         location == Plasma::Types::TopEdge         ? "top"
                             : location == Plasma::Types::LeftEdge  ? "left"
                             : location == Plasma::Types::RightEdge ? "right"
                                                                    : /* Plasma::Types::BottomEdge */ "bottom");

        const qreal height =
            // If we do not have a panel, fallback to 4 units
            !view ? 4 : (qreal)view->totalThickness() / gridUnit;

        panelJson.insert("height", height);
        if (view) {
            const auto alignment = view->alignment();
            panelJson.insert("maximumLength", (qreal)view->maximumLength() / gridUnit);
            panelJson.insert("minimumLength", (qreal)view->minimumLength() / gridUnit);
            panelJson.insert("offset", (qreal)view->offset() / gridUnit);
            panelJson.insert("alignment", alignment == Qt::AlignRight ? "right" : alignment == Qt::AlignCenter ? "center" : "left");
            switch (view->visibilityMode()) {
            case PanelView::AutoHide:
                panelJson.insert("hiding", "autohide");
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
        const QStringList appletsOrderStrings = genericConf.readEntry(QStringLiteral("AppletOrder"), QString()).split(QChar(';'));

        // Consider the applet order to be valid only if there are as many entries as applets()
        if (appletsOrderStrings.length() == cont->applets().length()) {
            for (const QString &appletId : appletsOrderStrings) {
                KConfigGroup appletConfig(&contConfig, QStringLiteral("Applets"));
                appletConfig = KConfigGroup(&appletConfig, appletId);

                const QString pluginName = appletConfig.readEntry(QStringLiteral("plugin"), QString());

                if (pluginName.isEmpty()) {
                    continue;
                }

                QJsonObject appletJson;

                appletConfig = KConfigGroup(&appletConfig, QStringLiteral("Configuration"));

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
        const QStringList appletsGeomStrings = genericConf.readEntry(QStringLiteral("ItemsGeometries"), QString()).split(QChar(';'));

        QHash<QString, QRect> appletGeometries;
        for (const QString &encoded : appletsGeomStrings) {
            const QStringList keyValue = encoded.split(QLatin1Char(':'));
            if (keyValue.length() != 2) {
                continue;
            }

            const QStringList rectPieces = keyValue.crbegin()->split(QLatin1Char(','));
            if (rectPieces.length() != 5) {
                continue;
            }

            appletGeometries.emplace(*keyValue.cbegin(), rectPieces[0].toInt(), rectPieces[1].toInt(), rectPieces[2].toInt(), rectPieces[3].toInt());
        }

        QJsonArray appletsJsonArray;

        const auto applets = cont->applets();
        for (Plasma::Applet *applet : applets) {
            const QRect geometry = appletGeometries.value(QStringLiteral("Applet-") % QString::number(applet->id()));

            QJsonObject appletJson;

            appletJson.insert("title", applet->title());
            appletJson.insert("plugin", applet->pluginMetaData().pluginId());

            appletJson.insert("geometry.x", geometry.x() / gridUnit);
            appletJson.insert("geometry.y", geometry.y() / gridUnit);
            appletJson.insert("geometry.width", geometry.width() / gridUnit);
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

    // get rid of old config
    const QStringList groupList = conf->groupList();
    for (const QString &group : groupList) {
        conf->deleteGroup(group);
    }
    conf->sync();
    unload();
    // Put load in queue of the event loop to wait for the whole set of containments to have been deleteLater(), as some like FolderView operate on singletons
    // which can cause inconsistent states
    QTimer::singleShot(0, this, &ShellCorona::load);
}

QString ShellCorona::shell() const
{
    return m_shell;
}

void ShellCorona::sanitizeScreenLayout(const QString &configFileName)
{
    KConfigGroup cg(KSharedConfig::openConfig(configFileName), QStringLiteral("Containments"));

    // The containment-> screen mappings we found in the config file
    QHash<QString, QMap<int, QString>> savedContainmentScreens;

    // Desktop containments with screen = -1 or duplicated wanting to go on the same screen as somebody else
    QStringList orphanContainments;
    // Panel containments we found we may want to remap the screen
    QStringList panelContainments;
    QSet<Plasma::Types::Location> panelLocations({Plasma::Types::TopEdge, Plasma::Types::BottomEdge, Plasma::Types::LeftEdge, Plasma::Types::RightEdge});

    for (const QString &idStr : cg.groupList()) {
        if (idStr.toInt() <= 0) {
            continue;
        }

        KConfigGroup contCg(&cg, idStr);
        int lastScreen = contCg.readEntry(QStringLiteral("lastScreen"), -1);

        if (panelLocations.contains(Plasma::Types::Location(contCg.readEntry(QStringLiteral("location"), 0)))) {
            if (lastScreen >= 0) {
                panelContainments.append(idStr);
            }
            continue;
        }

        const QString &activity = contCg.readEntry(QStringLiteral("activityId"), QString());

        if (lastScreen >= 0 && !savedContainmentScreens[activity].contains(lastScreen)) {
            savedContainmentScreens[activity][lastScreen] = idStr;
        } else {
            orphanContainments.append(idStr);
        }
    }

    QHash<int, int> screenMapping;

    // Ensure desktops screens are progressive
    for (auto activityIt = savedContainmentScreens.begin(); activityIt != savedContainmentScreens.end(); activityIt++) {
        const QString &activity = activityIt.key();
        int progressiveScreen = 0;
        for (auto originalScreenIt = activityIt.value().begin(); originalScreenIt != activityIt.value().end(); originalScreenIt++) {
            KConfigGroup contCg(&cg, originalScreenIt.value());
            screenMapping[originalScreenIt.key()] = progressiveScreen;
            contCg.writeEntry(QStringLiteral("lastScreen"), progressiveScreen++);
        }

        for (auto orphanContainmentsIt = orphanContainments.constBegin(); orphanContainmentsIt != orphanContainments.constEnd(); orphanContainmentsIt++) {
            KConfigGroup contCg(&cg, (*orphanContainmentsIt));
            const QString &orphanActivity = contCg.readEntry(QStringLiteral("activityId"), QString());
            if (orphanActivity == activity) {
                contCg.writeEntry(QStringLiteral("lastScreen"), progressiveScreen++);
            }
        }
    }

    // Remap panels to the screen changes we did for desktops
    for (auto panelsIt = panelContainments.begin(); panelsIt != panelContainments.end(); panelsIt++) {
        KConfigGroup contCg(&cg, (*panelsIt));
        int lastScreen = contCg.readEntry(QStringLiteral("lastScreen"), -1);

        // If we don't know where to put the panel, put it on the first screen
        contCg.writeEntry(QStringLiteral("lastScreen"), screenMapping.value(lastScreen, 0));
    }
}

void ShellCorona::load()
{
    if (m_shell.isEmpty()) {
        return;
    }

    auto activityStatus = m_activityController->serviceStatus();
    if (activityStatus != KActivities::Controller::Running && !qApp->property("org.kde.KActivities.core.disableAutostart").toBool()) {
        if (activityStatus == KActivities::Controller::NotRunning) {
            qCWarning(PLASMASHELL) << "Aborting shell load: The activity manager daemon (kactivitymanagerd) is not running.";
            qCWarning(PLASMASHELL)
                << "If this Plasma has been installed into a custom prefix, verify that its D-Bus services dir is known to the system for the daemon to be "
                   "activatable.";
        }
        return;
    }

    disconnect(m_activityController, &KActivities::Controller::serviceStatusChanged, this, &ShellCorona::load);

    // TODO: a kconf_update script is needed
    QString configFileName(QStringLiteral("plasma-") + m_shell + QStringLiteral("-appletsrc"));

    // Make sure all containments have screen numbers starting from 0 and are sequential
    sanitizeScreenLayout(configFileName);

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

        // Don't give a view to containments that don't want one (negative lastscreen)
        // (this is pretty mucha special case for the systray)
        // also, make sure we don't have a view already.
        // this will be true for first startup as the view has already been created at the new Panel JS call
        std::copy_if(containments.constBegin(), containments.constEnd(), std::back_inserter(m_waitingPanels), [this](Plasma::Containment *containment) {
            return ((containment->containmentType() == Plasma::Containment::Panel || containment->containmentType() == Plasma::Containment::CustomPanel)
                    && !m_waitingPanels.contains(containment) && containment->lastScreen() >= 0 && !m_panelViews.contains(containment));
        });
    }

    // NOTE: this is needed in case loadLayout() did *not* call loadDefaultLayout()
    // it needs to be after of loadLayout() as it would always create new
    // containments on each startup otherwise
    const auto screens = m_screenPool->screenOrder();
    for (QScreen *screen : screens) {
        QSet<QScreen *> managedScreens;
        for (auto *desk : m_desktopViewForScreen) {
            managedScreens.insert(desk->screenToFollow());
        }

        // the containments may have been created already by the startup script
        // check their existence in order to not have duplicated desktopviews
        if (!managedScreens.contains(screen)) {
            addOutput(screen);
        }
    }
    connect(m_screenPool, &ScreenPool::screenOrderChanged, this, &ShellCorona::handleScreenOrderChanged, Qt::UniqueConnection);
    connect(m_screenPool, &ScreenPool::screenRemoved, this, &ShellCorona::handleScreenRemoved, Qt::UniqueConnection);

    if (!m_waitingPanels.isEmpty()) {
        m_waitingPanelsTimer.start();
    }

    if (config()->isImmutable() || !KAuthorized::authorize(QStringLiteral("plasma/plasmashell/unlockedDesktop"))) {
        setImmutability(Plasma::Types::SystemImmutable);
    } else {
        KConfigGroup coronaConfig(config(), "General");
        setImmutability((Plasma::Types::ImmutabilityType)coronaConfig.readEntry("immutability", static_cast<int>(Plasma::Types::Mutable)));
    }
}

#ifndef NDEBUG
void ShellCorona::screenInvariants() const
{
    if (m_screenPool->noRealOutputsConnected()) {
        Q_ASSERT(m_desktopViewForScreen.isEmpty());
        Q_ASSERT(m_panelViews.isEmpty());
        return;
    }

    QSet<QScreen *> managedScreens;
    for (auto *desk : m_desktopViewForScreen) {
        managedScreens.insert(desk->screenToFollow());
    }

    Q_ASSERT(managedScreens.count() <= m_screenPool->screenOrder().count());

    QSet<QScreen *> screens;
    for (QScreen *knownScreen : managedScreens) {
        const int id = m_screenPool->idForScreen(knownScreen);
        const DesktopView *view = desktopForScreen(knownScreen);
        Q_ASSERT(view->isVisible());
        QScreen *screen = view->screenToFollow();
        Q_ASSERT(knownScreen == screen);
        Q_ASSERT(!screens.contains(screen));
        //         commented out because a different part of the code-base is responsible for this
        //         and sometimes is not yet called here.
        //         Q_ASSERT(!view->fillScreen() || view->geometry() == screen->geometry());

        Q_ASSERT(view->containment()->screen() == id || view->containment()->screen() == -1);
        Q_ASSERT(view->containment()->lastScreen() == id || view->containment()->lastScreen() == -1);
        Q_ASSERT(view->isVisible());

        for (const PanelView *panel : m_panelViews) {
            if (panel->screenToFollow() == screen) {
                Q_ASSERT(panel->containment());
                Q_ASSERT(panel->containment()->screen() == id || panel->containment()->screen() == -1);
                // If any kscreen related activities occurred
                // during startup, the panel wouldn't be visible yet, and this would assert
                if (panel->containment()->isUiReady()) {
                    Q_ASSERT(panel->isVisible());
                }
            }
        }

        screens.insert(screen);
    }

    if (m_desktopViewForScreen.isEmpty()) {
        qCWarning(PLASMASHELL) << "no screens!!";
    }
}
#endif

void ShellCorona::showAlternativesForApplet(Plasma::Applet *applet)
{
    const QUrl alternativesQML = kPackage().fileUrl("appletalternativesui");
    if (alternativesQML.isEmpty()) {
        return;
    }

    auto *qmlObj = new PlasmaQuick::SharedQmlEngine(this);
    qmlObj->setInitializationDelayed(true);
    qmlObj->setSource(alternativesQML);

    AlternativesHelper *helper = new AlternativesHelper(applet, qmlObj);
    qmlObj->rootContext()->setContextProperty(QStringLiteral("alternativesHelper"), helper);

    qmlObj->completeInitialization();

    auto dialog = qobject_cast<PlasmaQuick::Dialog *>(qmlObj->rootObject());
    if (!dialog) {
        qCWarning(PLASMASHELL) << "Alternatives UI does not inherit from Dialog";
        delete qmlObj;
        return;
    }
    connect(applet, &Plasma::Applet::destroyedChanged, qmlObj, [qmlObj](bool destroyed) {
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

    // Make double sure that we do not access the members while we are in the process of deleting them.
    // Most notably destroying PanelViews may issue signals that in turn cause iteration upon the m_panelViews.
    // First clear the members, then delete them.
    auto desktopViewForScreen = m_desktopViewForScreen;
    m_desktopViewForScreen.clear();
    qDeleteAll(desktopViewForScreen);
    auto panelViews = m_panelViews;
    m_panelViews.clear();
    qDeleteAll(m_panelViews);

    m_waitingPanels.clear();
    m_activityContainmentPlugins.clear();

    while (!containments().isEmpty()) {
        // Some applets react to destroyedChanged rather just destroyed,
        // give them  the possibility to react
        // deleting a containment will remove it from the list due to QObject::destroyed connect in Corona
        // this form doesn't crash, while qDeleteAll(containments()) does
        // And is more correct anyways to use destroy()
        containments().constFirst()->destroy();
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

void ShellCorona::slotCyclePanelFocus()
{
    if (m_panelViews.empty()) {
        return;
    }

    PanelView *activePanel = qobject_cast<PanelView *>(qGuiApp->focusWindow());
    if (!activePanel) {
        // Activate the first panel and save the previous window
        activePanel = m_panelViews.begin().value();
    }

    if (activePanel->containment()->status() != Plasma::Types::AcceptingInputStatus) {
        activePanel->containment()->setStatus(Plasma::Types::AcceptingInputStatus);
    } else {
        // Cancel focus on the current panel
        // Block focus on the panel if it's not the last panel
        if (activePanel != m_panelViews.last()) {
            m_blockRestorePreviousWindow = true;
        }
        activePanel->containment()->setStatus(Plasma::Types::PassiveStatus);
        m_blockRestorePreviousWindow = false;

        // More than one panel and the current panel is not the last panel,
        // move focus to next panel.
        if (activePanel != m_panelViews.last()) {
            auto viewIt = std::find_if(m_panelViews.cbegin(), m_panelViews.cend(), [activePanel](const PanelView *panel) {
                return activePanel == panel;
            });

            if (viewIt == m_panelViews.cend()) {
                return;
            }

            // Skip destroyed panels
            viewIt = std::next(viewIt);
            while (viewIt != m_panelViews.cend()) {
                if (!viewIt.value()->containment()->destroyed()) {
                    break;
                }

                viewIt = std::next(viewIt);
            }

            if (viewIt != m_panelViews.cend()) {
                viewIt.value()->containment()->setStatus(Plasma::Types::AcceptingInputStatus);
            } else {
                restorePreviousWindow();
            }
        }
    }
}

void ShellCorona::loadDefaultLayout()
{
#if USE_SCRIPTING
    // pre-startup scripts
    QString script = m_lookAndFeelPackage.filePath("layouts", shell() + "-prelayout.js");
    if (!script.isEmpty()) {
        QFile file(script);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString code = file.readAll();
            qCDebug(PLASMASHELL) << "evaluating pre-startup script:" << script;

            WorkspaceScripting::ScriptEngine scriptEngine(this);

            connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this, [](const QString &msg) {
                qCWarning(PLASMASHELL) << msg;
            });
            connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this, [](const QString &msg) {
                qCDebug(PLASMASHELL) << msg;
            });
            if (!scriptEngine.evaluateScript(code, script)) {
                qCWarning(PLASMASHELL) << "failed to initialize layout properly:" << script;
            }
        }
    }

    // NOTE: Is important the containments already exist for each screen
    // at the moment of the script execution,the same loop in :load()
    // is executed too late
    const auto screens = m_screenPool->screenOrder();
    for (QScreen *screen : screens) {
        addOutput(screen);
    }

    script = m_testModeLayout;

    if (script.isEmpty()) {
        script = m_lookAndFeelPackage.filePath("layouts", shell() + "-layout.js");
    }
    if (script.isEmpty()) {
        script = kPackage().filePath("defaultlayout");
    }

    QFile file(script);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString code = file.readAll();
        qCDebug(PLASMASHELL) << "evaluating startup script:" << script;

        // We need to know which activities are here in order for
        // the scripting engine to work. activityAdded does not mind
        // if we pass it the same activity multiple times
        const QStringList existingActivities = m_activityController->activities();
        for (const QString &id : existingActivities) {
            activityAdded(id);
        }

        WorkspaceScripting::ScriptEngine scriptEngine(this);

        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this, [](const QString &msg) {
            qCWarning(PLASMASHELL) << msg;
        });
        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this, [](const QString &msg) {
            qCDebug(PLASMASHELL) << msg;
        });
        if (!scriptEngine.evaluateScript(code, script)) {
            qCWarning(PLASMASHELL) << "failed to initialize layout properly:" << script;
        }
    }
#endif
    Q_EMIT startupCompleted();
}

void ShellCorona::processUpdateScripts()
{
#if USE_SCRIPTING
    const QStringList scripts = WorkspaceScripting::ScriptEngine::pendingUpdateScripts(this);
    if (scripts.isEmpty()) {
        return;
    }

    WorkspaceScripting::ScriptEngine scriptEngine(this);

    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this, [](const QString &msg) {
        qCWarning(PLASMASHELL) << msg;
    });
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this, [](const QString &msg) {
        qCDebug(PLASMASHELL) << msg;
    });

    for (const QString &script : scripts) {
        QFile file(script);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString code = file.readAll();
            scriptEngine.evaluateScript(code);
        } else {
            qCWarning(PLASMASHELL) << "Unable to open the script file" << script << "for reading";
        }
    }
#endif
}

int ShellCorona::numScreens() const
{
    return m_screenPool->screenOrder().count();
}

QRect ShellCorona::screenGeometry(int id) const
{
    QScreen *screen = m_screenPool->screenForId(id);
    if (!screen) {
        qCWarning(PLASMASHELL) << "requesting unexisting screen geometry" << id;
        screen = m_screenPool->primaryScreen();
        return screen ? screen->geometry() : QRect();
    }
    return screen->geometry();
}

QRegion ShellCorona::availableScreenRegion(int id) const
{
    return m_strutManager->availableScreenRegion(id);
}

QRegion ShellCorona::_availableScreenRegion(int id) const
{
    QScreen *screen = m_screenPool->screenForId(id);
    if (!screen) {
        // each screen should have a view
        qCWarning(PLASMASHELL) << "requesting unexisting screen region" << id;
        screen = m_screenPool->primaryScreen();
        return screen ? screen->availableGeometry() : QRegion();
    }

    return std::accumulate(m_panelViews.cbegin(), m_panelViews.cend(), QRegion(screen->geometry()), [screen](const QRegion &a, const PanelView *v) {
        if (v->isVisible() && screen == v->screen() && v->visibilityMode() != PanelView::AutoHide) {
            // if the panel is being moved around, we still want to calculate it from the edge
            return a - v->geometryByDistance(0);
        }
        return a;
    });
}

QRect ShellCorona::availableScreenRect(int id) const
{
    return m_strutManager->availableScreenRect(id);
}

QRect ShellCorona::_availableScreenRect(int id) const
{
    QScreen *screen = m_screenPool->screenForId(id);
    if (!screen) {
        // each screen should have a view
        qCWarning(PLASMASHELL) << "requesting unexisting screen available rect" << id;
        screen = m_screenPool->primaryScreen();
        return screen ? screen->availableGeometry() : QRect();
    }

    QRect r = screen->geometry();
    int topThickness, leftThickness, rightThickness, bottomThickness;
    topThickness = leftThickness = rightThickness = bottomThickness = 0;
    for (PanelView *v : m_panelViews) {
        if (v->isVisible() && v->screen() == screen && v->visibilityMode() != PanelView::AutoHide) {
            switch (v->location()) {
            case Plasma::Types::LeftEdge:
                leftThickness = qMax(leftThickness, v->totalThickness());
                break;
            case Plasma::Types::RightEdge:
                rightThickness = qMax(rightThickness, v->totalThickness());
                break;
            case Plasma::Types::TopEdge:
                topThickness = qMax(topThickness, v->totalThickness());
                break;
            case Plasma::Types::BottomEdge:
                bottomThickness = qMax(bottomThickness, v->totalThickness());
            default:
                break;
            }
        }
    }
    r.setLeft(r.left() + leftThickness);
    r.setRight(r.right() - rightThickness);
    r.setTop(r.top() + topThickness);
    r.setBottom(r.bottom() - bottomThickness);
    return r;
}

QStringList ShellCorona::availableActivities() const
{
    return m_activityContainmentPlugins.keys();
}

void ShellCorona::removeDesktop(DesktopView *desktopView)
{
    const int screenId = desktopView->containment()->lastScreen();

    auto result = std::find_if(m_desktopViewForScreen.begin(), m_desktopViewForScreen.end(), [desktopView](DesktopView *v) {
        return v == desktopView;
    });

    if (result != m_desktopViewForScreen.end()) {
        m_desktopViewForScreen.erase(result);
    }

    QMutableMapIterator<const Plasma::Containment *, PanelView *> it(m_panelViews);

    while (it.hasNext()) {
        it.next();
        PanelView *panelView = it.value();

        if (panelView->containment()->lastScreen() == screenId) {
            m_waitingPanels << panelView->containment();
            it.remove();
            panelView->destroy();
            panelView->containment()->reactToScreenChange();
        }
    }

    desktopView->destroy();
    desktopView->containment()->reactToScreenChange();
}

PanelView *ShellCorona::panelView(Plasma::Containment *containment) const
{
    return m_panelViews.value(containment);
}

void ShellCorona::savePreviousWindow()
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11() && m_previousWId == 0) {
        m_previousWId = KX11Extras::activeWindow();
    }
#endif
    if (m_waylandWindowManagement && !m_previousPlasmaWindow) {
        m_previousPlasmaWindow = m_waylandWindowManagement->activeWindow();
    }
}

void ShellCorona::restorePreviousWindow()
{
    if (m_blockRestorePreviousWindow) {
        return;
    }

#if HAVE_X11
    if (KWindowSystem::isPlatformX11() && m_previousWId) {
        KX11Extras::forceActiveWindow(m_previousWId);
    }
#endif
    if (m_previousPlasmaWindow) {
        m_previousPlasmaWindow->requestActivate();
    }

    clearPreviousWindow();
}

void ShellCorona::clearPreviousWindow()
{
    m_previousWId = 0;
    m_previousPlasmaWindow = nullptr;
}

///// SLOTS

DesktopView *ShellCorona::desktopForScreen(QScreen *screen) const
{
    if (!screen) {
        return nullptr;
    }
    if (auto *v = m_desktopViewForScreen.value(m_screenPool->idForScreen(screen))) {
        return v;
    }

    return nullptr;
}

void ShellCorona::handleScreenRemoved(QScreen *screen)
{
    if (DesktopView *v = desktopForScreen(screen)) {
        // Checking with m_screenPool->screenOrder().count() - 1 because when ScreenPool emits screenRemoved, the screen has not yet been removed from
        // ScreenPool::screenORder()
        if (v->containment()->lastScreen() < 0 || v->containment()->lastScreen() >= m_screenPool->screenOrder().count() - 1) {
            removeDesktop(v);
        }
        // Else do nothing, the view will be recycled
    }

    // The real screen index that has been removed is *always* the highest one, because we enforce order.
    // There can't be a containment that has for instance screen 0 and another 2 but nothing on 1
    // It's size() - 1 because at this point screenpool didn't remove it from screenOrder() yet
    Q_EMIT screenRemoved(m_screenPool->screenOrder().size() - 1);
#ifndef NDEBUG
    m_invariantsTimer.start();
#endif
}

void ShellCorona::handleScreenOrderChanged(QList<QScreen *> screens)
{
    m_screenReorderInProgress = true;
    // First: reassign existing views if applicable, otherwise remove them
    auto allDesktops = m_desktopViewForScreen.values();
    m_desktopViewForScreen.clear();
    for (auto *v : allDesktops) {
        const int screenNumber = v->containment()->lastScreen();
        if (screenNumber >= 0 && screenNumber < screens.count()) {
            v->setScreenToFollow(screens[screenNumber]);
            v->setVisible(true);
            m_desktopViewForScreen[screenNumber] = v;
        } else {
            removeDesktop(v);
        }
    }

    // Doing it here as m_panelViews might already have been modified by the step before
    auto allPanels = m_panelViews.values();
    m_panelViews.clear();
    for (auto *v : allPanels) {
        const int screenNumber = v->containment()->lastScreen();
        if (screenNumber >= 0 && screenNumber < screens.count()) {
            v->setScreenToFollow(screens[screenNumber]);
            v->setVisible(true);
            m_panelViews[v->containment()] = v;
        } else {
            v->destroy();
            v->containment()->reactToScreenChange();
        }
    }

    // Second: add everything that wasn't there yet
    for (auto *s : screens) {
        if (!desktopForScreen(s)) {
            addOutput(s);
        }
    }

    m_screenReorderInProgress = false;
    Q_EMIT screenOrderChanged(screens);

    Q_ASSERT(m_desktopViewForScreen.count() == screens.count());
    for (int i = 0; i < screens.count(); ++i) {
        Q_EMIT screenGeometryChanged(i);
        Q_EMIT availableScreenRectChanged(i);
    }

    CHECK_SCREEN_INVARIANTS
}

void ShellCorona::addOutput(QScreen *screen)
{
    Q_ASSERT(screen);
    if (desktopForScreen(screen)) {
        Q_EMIT screenAdded(m_screenPool->idForScreen(screen));
        return;
    }
    Q_ASSERT(!screen->geometry().isNull());
#ifndef NDEBUG
    connect(screen, &QScreen::geometryChanged, &m_invariantsTimer, static_cast<void (QTimer::*)()>(&QTimer::start), Qt::UniqueConnection);
#endif
    int insertPosition = m_screenPool->idForScreen(screen);
    Q_ASSERT(insertPosition >= 0);

    DesktopView *view = new DesktopView(this, screen);

    if (view->rendererInterface()->graphicsApi() != QSGRendererInterface::Software) {
        connect(view, &QQuickWindow::sceneGraphError, this, &ShellCorona::glInitializationFailed);
    }
    connect(view, &DesktopView::geometryChanged, this, [this, view]() {
        const int id = m_screenPool->idForScreen(view->screen());
        if (id >= 0 && !m_screenReorderInProgress) {
            Q_EMIT screenGeometryChanged(id);
            Q_EMIT availableScreenRectChanged(id);
        }
    });

    Plasma::Containment *containment = createContainmentForActivity(m_activityController->currentActivity(), insertPosition);
    Q_ASSERT(containment);

    QAction *removeAction = containment->internalAction(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }

    connect(containment, &Plasma::Containment::uiReadyChanged, this, &ShellCorona::checkAllDesktopsUiReady);

    m_desktopViewForScreen[insertPosition] = view;
    view->setContainment(containment);
    view->show();

    Q_ASSERT(screen == view->screen());

    // need to specifically call the reactToScreenChange, since when the screen is shown it's not yet
    // in the list. We still don't want to have an invisible view added.
    containment->reactToScreenChange();

    // were there any panels for this screen before it popped up?
    if (!m_waitingPanels.isEmpty()) {
        m_waitingPanelsTimer.start();
    }

    if (!m_screenReorderInProgress) {
        Q_EMIT availableScreenRectChanged(m_screenPool->idForScreen(screen));
    }
    Q_EMIT screenAdded(m_screenPool->idForScreen(screen));
#ifndef NDEBUG
    m_invariantsTimer.start();
#endif
}

void ShellCorona::checkAllDesktopsUiReady(bool ready)
{
    if (!ready)
        return;
    for (auto v : qAsConst(m_desktopViewForScreen)) {
        if (!v->containment()->isUiReady())
            return;

        qCDebug(PLASMASHELL) << "Plasma Shell startup completed";
        QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                                                             QStringLiteral("/KSplash"),
                                                                             QStringLiteral("org.kde.KSplash"),
                                                                             QStringLiteral("setStage"));
        ksplashProgressMessage.setArguments(QList<QVariant>() << QStringLiteral("desktop"));
        QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
    }
}

Plasma::Containment *ShellCorona::createContainmentForActivity(const QString &activity, int screenNum)
{
    Plasma::Containment *lastScreenCont = nullptr;
    Plasma::Containment *orphanCont = nullptr;
    const auto containments = containmentsForActivity(activity);
    for (Plasma::Containment *cont : containments) {
        // in the case of a corrupt config file
        // with multiple containments with same lastScreen
        // Always prefer a containment that already has a screen, if any
        // This piece of code always fails at startup, used only later in plasma runtime
        if (cont->destroyed()) {
            continue;
        }
        if (cont->screen() == screenNum) {
            // Always prefer a containment that already has a view
            return cont;
        } else if (cont->lastScreen() == screenNum) {
            // Otherwise base off lastScreen
            lastScreenCont = cont;
        } else if (cont->lastScreen() < 0) {
            // Last resort, if we found a desktop for the activity that for whatever reason had screen -1 (very unlikely) recycle it
            orphanCont = cont;
        }
    }
    if (lastScreenCont) {
        return lastScreenCont;
    } else if (orphanCont) {
        return orphanCont;
    }

    QString plugin = m_activityContainmentPlugins.value(activity);

    if (plugin.isEmpty()) {
        plugin = defaultContainmentPlugin();
    }

    Plasma::Containment *containment = containmentForScreen(screenNum, activity, plugin, QVariantList());
    Q_ASSERT(containment);

    return containment;
}

void ShellCorona::createWaitingPanels()
{
    QList<Plasma::Containment *> stillWaitingPanels;

    for (Plasma::Containment *cont : qAsConst(m_waitingPanels)) {
        // ignore non existing (yet?) screens
        int requestedScreen = cont->lastScreen();
        if (requestedScreen < 0) {
            requestedScreen = 0;
        }

        QScreen *screen = m_screenPool->screenForId(requestedScreen);
        DesktopView *desktopView = desktopForScreen(screen);
        if (!screen || !desktopView) {
            stillWaitingPanels << cont;
            continue;
        }

        // TODO: does a similar check make sense?
        // Q_ASSERT(qBound(0, requestedScreen, m_screenPool->count() - 1) == requestedScreen);
        PanelView *panel = new PanelView(this, screen);
        if (panel->rendererInterface()->graphicsApi() != QSGRendererInterface::Software) {
            connect(panel, &QQuickWindow::sceneGraphError, this, &ShellCorona::glInitializationFailed);
        }
        auto rectNotify = [this, panel]() {
            if (!m_screenReorderInProgress && panel->containment()) {
                Q_EMIT availableScreenRectChanged(panel->containment()->screen());
            }
        };

        m_panelViews[cont] = panel;
        panel->setContainment(cont);
        cont->reactToScreenChange();

        rectNotify();

        connect(cont, &QObject::destroyed, this, &ShellCorona::panelContainmentDestroyed);

        connect(panel, &QWindow::visibleChanged, this, rectNotify);
        connect(panel, &QWindow::screenChanged, this, rectNotify);
        connect(panel, &PanelView::locationChanged, this, rectNotify);
        connect(panel, &PanelView::visibilityModeChanged, this, rectNotify);
        connect(panel, &PanelView::thicknessChanged, this, rectNotify);
    }
    m_waitingPanels = stillWaitingPanels;
}

void ShellCorona::panelContainmentDestroyed(QObject *obj)
{
    auto *cont = static_cast<Plasma::Containment *>(obj);
    int screen = cont->screen();
    auto view = m_panelViews.take(cont);
    delete view;
    // don't make things relayout when the application is quitting
    // NOTE: qApp->closingDown() is still false here
    if (!m_closingDown && !m_screenReorderInProgress) {
        Q_EMIT availableScreenRectChanged(screen);
    }
}

void ShellCorona::handleContainmentAdded(Plasma::Containment *c)
{
    connect(c, &Plasma::Containment::showAddWidgetsInterface, this, &ShellCorona::toggleWidgetExplorer);
    connect(c, &Plasma::Containment::appletAlternativesRequested, this, &ShellCorona::showAlternativesForApplet);
    connect(c, &Plasma::Containment::appletCreated, this, [this, c](Plasma::Applet *applet) {
        executeSetupPlasmoidScript(c, applet);
    });

    // When a containment is removed, remove the thumbnail as well
    connect(c, &Plasma::Containment::destroyedChanged, this, [this, c](bool destroyed) {
        if (!destroyed) {
            return;
        }
        const QString snapshotPath = containmentPreviewPath(c);
        if (!snapshotPath.isEmpty()) {
            QFile f(snapshotPath);
            f.remove();
        }
    });
}

void ShellCorona::executeSetupPlasmoidScript(Plasma::Containment *containment, Plasma::Applet *applet)
{
#if USE_SCRIPTING
    if (!applet->pluginMetaData().isValid() || !containment->pluginMetaData().isValid()) {
        return;
    }

    const QString scriptFile = m_lookAndFeelPackage.filePath("plasmoidsetupscripts", applet->pluginMetaData().pluginId() + ".js");

    if (scriptFile.isEmpty()) {
        return;
    }

    WorkspaceScripting::ScriptEngine scriptEngine(this);

    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this, [](const QString &msg) {
        qCWarning(PLASMASHELL) << msg;
    });
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this, [](const QString &msg) {
        qCDebug(PLASMASHELL) << msg;
    });

    QFile file(scriptFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(PLASMASHELL) << "Unable to load script file:" << scriptFile;
        return;
    }

    QString script = file.readAll();
    if (script.isEmpty()) {
        // qCDebug(PLASMASHELL) << "script is empty";
        return;
    }

    scriptEngine.globalObject().setProperty(QStringLiteral("applet"), scriptEngine.wrap(applet));
    scriptEngine.globalObject().setProperty(QStringLiteral("containment"), scriptEngine.wrap(containment));
    scriptEngine.evaluateScript(script, scriptFile);
#endif
}

void ShellCorona::toggleWidgetExplorer()
{
    // FIXME: This does not work on wayland
    const QPoint cursorPos = QCursor::pos();
    for (DesktopView *view : qAsConst(m_desktopViewForScreen)) {
        if (view->screen()->geometry().contains(cursorPos)) {
            // The view QML has to provide something to display the widget explorer
            view->rootObject()->metaObject()->invokeMethod(view->rootObject(), "toggleWidgetExplorer", Q_ARG(QVariant, QVariant::fromValue(sender())));
            return;
        }
    }
}

void ShellCorona::toggleActivityManager()
{
    const QPoint cursorPos = QCursor::pos();
    for (DesktopView *view : qAsConst(m_desktopViewForScreen)) {
        if (view->screen()->geometry().contains(cursorPos)) {
            // The view QML has to provide something to display the activity explorer
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

void ShellCorona::handleColorRequestedFromDBus(const QDBusMessage &msg)
{
    Q_ASSERT(!m_accentColorFromWallpaperEnabled);
    Q_ASSERT(!m_fakeColorRequestConn);
    msg.setDelayedReply(true);

    m_fakeColorRequestConn = connect(this, &ShellCorona::colorChanged, this, [this, msg] {
        disconnect(m_fakeColorRequestConn);
        const QRgb color = desktopForScreen(m_screenPool->primaryScreen())->accentColor().rgba();

        m_accentColorFromWallpaperEnabled = false;
        Q_EMIT accentColorFromWallpaperEnabledChanged();

        const QDBusMessage reply = msg.createReply(color);
        QDBusConnection::sessionBus().send(reply);
    });

    m_accentColorFromWallpaperEnabled = true;
    Q_EMIT accentColorFromWallpaperEnabledChanged();
}

QRgb ShellCorona::color() const
{
    // Colors from wallpaper are not generated when they are turned off in the settings.
    // To return a color we need to fake that the setting is on, and then take the color,
    // turn off the setting again(or the color engine will keep runnig) and return the color.

    // Note that whenever a color is generated, it is also set as accent color. When we fake the
    // setting, we should not apply the generated color. The color applying kded module take care
    // of that by checking for the original state of the setting, so it is important that the check
    // may not be removed accidentally.

    static QRgb defaultColor = QColor(Qt::transparent).rgba();
    auto const primaryDesktopViewExists = desktopForScreen(m_screenPool->primaryScreen());
    if (!primaryDesktopViewExists) {
        return defaultColor;
    }

    if (m_accentColorFromWallpaperEnabled) {
        return desktopForScreen(m_screenPool->primaryScreen())->accentColor().rgba();
    } else if (calledFromDBus()) {
        if (m_fakeColorRequestConn) {
            disconnect(m_fakeColorRequestConn);
        }
        const_cast<ShellCorona *>(this)->handleColorRequestedFromDBus(message());
    }

    return defaultColor;
}

QString ShellCorona::evaluateScript(const QString &script)
{
#if USE_SCRIPTING
    if (calledFromDBus()) {
        if (immutability() == Plasma::Types::SystemImmutable) {
            sendErrorReply(QDBusError::Failed, QStringLiteral("Widgets are locked"));
            return QString();
        } else if (!KAuthorized::authorize(QStringLiteral("plasma-desktop/scripting_console"))) {
            sendErrorReply(QDBusError::Failed, QStringLiteral("Administrative policies prevent script execution"));
            return QString();
        }
    }

    WorkspaceScripting::ScriptEngine scriptEngine(this);
    QString buffer;
    QTextStream bufferStream(&buffer, QIODevice::WriteOnly | QIODevice::Text);

    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this, [&bufferStream](const QString &msg) {
        qCWarning(PLASMASHELL) << msg;
        bufferStream << msg;
    });
    connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this, [&bufferStream](const QString &msg) {
        qCDebug(PLASMASHELL) << msg;
        bufferStream << msg;
    });

    scriptEngine.evaluateScript(script);

    bufferStream.flush();

    if (calledFromDBus() && !scriptEngine.errorString().isEmpty()) {
        sendErrorReply(QDBusError::Failed, scriptEngine.errorString());
        return QString();
    }

    return buffer;
#else
    return QString();
#endif
}

void ShellCorona::checkActivities()
{
    KActivities::Controller::ServiceStatus status = m_activityController->serviceStatus();
    // qCDebug(PLASMASHELL) << "$%$%$#%$%$%Status:" << status;
    if (status != KActivities::Controller::Running) {
        // panic and give up - better than causing a mess
        qCDebug(PLASMASHELL) << "ShellCorona::checkActivities is called whilst activity daemon is still connecting";
        return;
    }

    const QStringList existingActivities = m_activityController->activities();
    for (const QString &id : existingActivities) {
        activityAdded(id);
    }

    // Checking whether the result we got is valid. Just in case.
    Q_ASSERT_X(!existingActivities.isEmpty(), "isEmpty", "There are no activities, and the service is running");
    Q_ASSERT_X(existingActivities[0] != QLatin1String("00000000-0000-0000-0000-000000000000"), "null uuid", "There is a nulluuid activity present");

    // Killing the unassigned containments
    const auto conts = containments();
    for (Plasma::Containment *cont : conts) {
        if ((cont->containmentType() == Plasma::Containment::Desktop || cont->containmentType() == Plasma::Containment::Custom)
            && !existingActivities.contains(cont->activity()) && m_activityController->currentActivity() != cont->activity()) {
            cont->destroy();
        }
    }
}

void ShellCorona::currentActivityChanged(const QString &newActivity)
{
    //     qCDebug(PLASMASHELL) << "Activity changed:" << newActivity;

    for (auto it = m_desktopViewForScreen.constBegin(); it != m_desktopViewForScreen.constEnd(); ++it) {
        Plasma::Containment *c = createContainmentForActivity(newActivity, it.key());

        QAction *removeAction = c->internalAction(QStringLiteral("remove"));
        if (removeAction) {
            removeAction->deleteLater();
        }
        (*it)->setContainment(c);
    }
}

void ShellCorona::activityAdded(const QString &id)
{
    // TODO more sanity checks
    if (m_activityContainmentPlugins.contains(id)) {
        qCWarning(PLASMASHELL) << "Activity added twice" << id;
        return;
    }

    m_activityContainmentPlugins.insert(id, defaultContainmentPlugin());
}

void ShellCorona::activityRemoved(const QString &id)
{
    m_activityContainmentPlugins.remove(id);
    const QList<Plasma::Containment *> containments = containmentsForActivity(id);
    for (auto cont : containments) {
        cont->destroy();
    }
}

void ShellCorona::insertActivity(const QString &id, const QString &plugin)
{
    activityAdded(id);

    // TODO: This needs to go away!
    // The containment creation API does not know when we have a
    // new activity to create a containment for, we need to pretend
    // that the current activity has been changed
    QFuture<bool> currentActivity = m_activityController->setCurrentActivity(id);
    awaitFuture(currentActivity);

    if (!currentActivity.result()) {
        qCDebug(PLASMASHELL) << "Failed to create and switch to the activity";
        return;
    }

    while (m_activityController->currentActivity() != id) {
        QCoreApplication::processEvents();
    }

    m_activityContainmentPlugins.insert(id, plugin);
    for (auto it = m_desktopViewForScreen.constBegin(); it != m_desktopViewForScreen.constEnd(); ++it) {
        Plasma::Containment *c = createContainmentForActivity(id, (*it)->containment()->screen());
        if (c) {
            c->config().writeEntry("lastScreen", (*it)->containment()->screen());
        }
    }
}

Plasma::Containment *ShellCorona::setContainmentTypeForScreen(int screen, const QString &plugin)
{
    // search but not create
    Plasma::Containment *oldContainment = containmentForScreen(screen, m_activityController->currentActivity(), QString());

    // no valid containment in given screen, giving up
    if (!oldContainment) {
        return nullptr;
    }

    if (plugin.isEmpty()) {
        return oldContainment;
    }

    auto viewIt = std::find_if(m_desktopViewForScreen.cbegin(), m_desktopViewForScreen.cend(), [oldContainment](const DesktopView *v) {
        return v->containment() == oldContainment;
    });

    // no view? give up
    if (viewIt == m_desktopViewForScreen.cend()) {
        return oldContainment;
    }

    // create a new containment
    Plasma::Containment *newContainment = createContainmentDelayed(plugin);

    // if creation failed or invalid plugin, give up
    if (!newContainment) {
        return oldContainment;
    } else if (!newContainment->pluginMetaData().isValid()) {
        newContainment->deleteLater();
        return oldContainment;
    }

    // At this point we have a valid new containment from plugin and a view
    // copy all configuration groups (excluded applets)
    KConfigGroup oldCg = oldContainment->config();

    // newCg *HAS* to be from a KSharedConfig, because some KConfigSkeleton will need to be synced
    // this makes the configscheme work
    KConfigGroup newCg(KSharedConfig::openConfig(oldCg.config()->name()), "Containments");
    newCg = KConfigGroup(&newCg, QString::number(newContainment->id()));

    // this makes containment->config() work, is a separate thing from its configscheme
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
    newCg.writeEntry("wallpaperplugin", oldContainment->wallpaper());
    newContainment->restore(newCg);
    newContainment->updateConstraints(Plasma::Types::StartupCompletedConstraint);
    newContainment->flushPendingConstraintsEvents();
    Q_EMIT containmentAdded(newContainment);

    // Move the applets
    const auto applets = oldContainment->applets();
    for (Plasma::Applet *applet : applets) {
        newContainment->addApplet(applet);
    }

    // remove the "remove" action
    QAction *removeAction = newContainment->internalAction(QStringLiteral("remove"));
    if (removeAction) {
        removeAction->deleteLater();
    }
    (*viewIt)->setContainment(newContainment);
    newContainment->setActivity(oldContainment->activity());

    oldContainment->destroy();

    // removing the focus from the item that is going to be destroyed
    // fixes a crash
    // delayout the destruction of the old containment fixes another crash
    (*viewIt)->rootObject()->setFocus(true, Qt::MouseFocusReason);
    QTimer::singleShot(2500, oldContainment, &Plasma::Applet::destroy);

    // Save now as we now have a screen, so lastScreen will not be -1
    newContainment->save(newCg);
    requestConfigSync();
    Q_EMIT availableScreenRectChanged(screen);

    return newContainment;
}

void ShellCorona::checkAddPanelAction()
{
    delete m_addPanelAction;
    m_addPanelAction = nullptr;

    m_addPanelsMenu.reset(nullptr);

    const QList<KPluginMetaData> panelContainmentPlugins = Plasma::PluginLoader::listContainmentsMetaDataOfType(QStringLiteral("Panel"));

    auto filter = [](const KPluginMetaData &md) -> bool {
        return !md.rawData().value(QStringLiteral("NoDisplay")).toBool()
            && md.value(QStringLiteral("X-Plasma-ContainmentCategories"), QStringList()).contains(QLatin1String("panel"));
    };
    QList<KPluginMetaData> templates = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), filter);

    if (panelContainmentPlugins.count() + templates.count() == 1) {
        m_addPanelAction = new QAction(this);
        connect(m_addPanelAction, &QAction::triggered, this, qOverload<>(&ShellCorona::addPanel));
    } else if (!panelContainmentPlugins.isEmpty()) {
        m_addPanelAction = new QAction(this);
        m_addPanelsMenu.reset(new QMenu);
        m_addPanelAction->setMenu(m_addPanelsMenu.get());
        connect(m_addPanelsMenu.get(), &QMenu::aboutToShow, this, &ShellCorona::populateAddPanelsMenu);
        connect(m_addPanelsMenu.get(), &QMenu::triggered, this, qOverload<QAction *>(&ShellCorona::addPanel));
    }

    if (m_addPanelAction) {
        m_addPanelAction->setText(i18n("Add Panel"));
        m_addPanelAction->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
        setAction(QStringLiteral("add panel"), m_addPanelAction);
    }
}

void ShellCorona::populateAddPanelsMenu()
{
    m_addPanelsMenu->clear();
    const KPluginMetaData emptyInfo;

    const QList<KPluginMetaData> panelContainmentPlugins = Plasma::PluginLoader::listContainmentsMetaDataOfType(QStringLiteral("Panel"));
    QMap<QString, QPair<KPluginMetaData, KPluginMetaData>> sorted;
    for (const KPluginMetaData &plugin : panelContainmentPlugins) {
        if (plugin.rawData().value(QStringLiteral("NoDisplay")).toBool()) {
            continue;
        }
        sorted.insert(plugin.name(), qMakePair(plugin, KPluginMetaData()));
    }

    auto filter = [](const KPluginMetaData &md) -> bool {
        return !md.rawData().value(QStringLiteral("NoDisplay")).toBool()
            && md.value(QStringLiteral("X-Plasma-ContainmentCategories"), QStringList()).contains(QLatin1String("panel"));
    };
    const QList<KPluginMetaData> templates = KPackage::PackageLoader::self()->findPackages(QStringLiteral("Plasma/LayoutTemplate"), QString(), filter);
    for (const auto &tpl : templates) {
        sorted.insert(tpl.name(), qMakePair(emptyInfo, tpl));
    }

    QMapIterator<QString, QPair<KPluginMetaData, KPluginMetaData>> it(sorted);
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LayoutTemplate"));
    while (it.hasNext()) {
        it.next();
        QPair<KPluginMetaData, KPluginMetaData> pair = it.value();
        if (pair.first.isValid()) {
            KPluginMetaData plugin = pair.first;
            QAction *action = m_addPanelsMenu->addAction(i18nc("Creates an empty containment (%1 is the containment name)", "Empty %1", plugin.name()));
            if (!plugin.iconName().isEmpty()) {
                action->setIcon(QIcon::fromTheme(plugin.iconName()));
            }

            action->setData(plugin.pluginId());
        } else {
            KPluginMetaData plugin(pair.second);
            package.setPath(plugin.pluginId());
            const QString scriptFile = package.filePath("mainscript");
            if (!scriptFile.isEmpty()) {
                QAction *action = m_addPanelsMenu->addAction(plugin.name());
                action->setData(QStringLiteral("plasma-desktop-template:%1").arg(plugin.pluginId()));
            }
        }
    }
}

void ShellCorona::addPanel()
{
    const QList<KPluginMetaData> panelPlugins = Plasma::PluginLoader::listContainmentsMetaDataOfType(QStringLiteral("Panel"));

    if (!panelPlugins.isEmpty()) {
        addPanel(panelPlugins.first().pluginId());
    }
}

void ShellCorona::addPanel(QAction *action)
{
    const QString plugin = action->data().toString();
#if USE_SCRIPTING
    if (plugin.startsWith(QLatin1String("plasma-desktop-template:"))) {
        WorkspaceScripting::ScriptEngine scriptEngine(this);

        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::printError, this, [](const QString &msg) {
            qCWarning(PLASMASHELL) << msg;
        });
        connect(&scriptEngine, &WorkspaceScripting::ScriptEngine::print, this, [](const QString &msg) {
            qCDebug(PLASMASHELL) << msg;
        });
        const QString templateName = plugin.right(plugin.length() - qstrlen("plasma-desktop-template:"));

        scriptEngine.evaluateScript(QStringLiteral("loadTemplate(\"%1\")").arg(templateName));
    } else
#endif
        if (!plugin.isEmpty()) {
        addPanel(plugin);
    }
}

Plasma::Containment *ShellCorona::addPanel(const QString &plugin)
{
    Plasma::Containment *panel = createContainment(plugin);
    if (!panel) {
        return nullptr;
    }

    // find out what screen this panel should go on
    QScreen *wantedScreen = qGuiApp->focusWindow() ? qGuiApp->focusWindow()->screen() : m_screenPool->primaryScreen();

    QList<Plasma::Types::Location> availableLocations;
    availableLocations << Plasma::Types::BottomEdge << Plasma::Types::TopEdge << Plasma::Types::LeftEdge << Plasma::Types::RightEdge;

    for (auto it = m_panelViews.constBegin(); it != m_panelViews.constEnd(); ++it) {
        if ((*it)->screenToFollow() == wantedScreen) {
            availableLocations.removeAll((*it)->location());
        }
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
    // immediately create the panel here so that we have access to the panel view
    createWaitingPanels();

    if (m_panelViews.contains(panel)) {
        m_panelViews.value(panel)->setScreenToFollow(wantedScreen);
    }

    return panel;
}

void ShellCorona::swapDesktopScreens(int oldScreen, int newScreen)
{
    for (auto *containment : containmentsForScreen(oldScreen)) {
        if (containment->containmentType() != Plasma::Containment::Panel && containment->containmentType() != Plasma::Containment::CustomPanel) {
            setScreenForContainment(containment, newScreen);
        }
    }
}

void ShellCorona::setScreenForContainment(Plasma::Containment *containment, int newScreenId)
{
    const int oldScreenId = containment->screen() >= 0 ? containment->screen() : containment->lastScreen();

    if (oldScreenId == newScreenId) {
        return;
    }

    m_pendingScreenChanges[containment] = newScreenId;

    if (containment->containmentType() == Plasma::Containment::Panel || containment->containmentType() == Plasma::Containment::CustomPanel) {
        // Panel Case
        containment->reactToScreenChange();
        auto *panelView = m_panelViews.value(containment);
        // If newScreen != nullptr we are also assured it won't be redundant
        QScreen *newScreen = m_screenPool->screenForId(newScreenId);

        if (panelView) {
            // There was an existing panel view
            if (newScreen) {
                panelView->setScreenToFollow(newScreen);
            } else {
                // Not on a connected screen: destroy the panel
                if (!m_waitingPanels.contains(containment)) {
                    m_waitingPanels << containment;
                }
                m_panelViews.remove(containment);
                panelView->destroy();
            }
        } else {
            // Didn't have a view, createWaitingPanels() will create it if needed
            createWaitingPanels();
        }

    } else {
        // Desktop case: a bit more complicate because we may have to swap
        Plasma::Containment *contSwap = containmentForScreen(newScreenId, containment->activity(), "org.kde.plasma.folder");
        Q_ASSERT(contSwap);

        // Perform the lastScreen changes
        containment->reactToScreenChange();
        if (contSwap) {
            m_pendingScreenChanges[contSwap] = oldScreenId;
            contSwap->reactToScreenChange();
        }

        // If those will be != nullptr we are also assured they won't be redundant
        QScreen *oldScreen = m_screenPool->screenForId(oldScreenId);
        QScreen *newScreen = m_screenPool->screenForId(newScreenId);

        // Actually perform a view swap when we are dealing with containments of current activity
        if (contSwap && containment->activity() == m_activityController->currentActivity()) {
            auto *containmentView = m_desktopViewForScreen.value(oldScreenId);
            auto *contSwapView = m_desktopViewForScreen.value(newScreenId);

            if (containmentView) {
                Q_ASSERT(containment == containmentView->containment());
                m_desktopViewForScreen.remove(oldScreenId);
            }
            if (contSwapView) {
                Q_ASSERT(contSwap == contSwapView->containment());
                m_desktopViewForScreen.remove(newScreenId);
            }

            if (containmentView) {
                if (newScreen) {
                    containmentView->setScreenToFollow(newScreen);
                    m_desktopViewForScreen[newScreenId] = containmentView;
                } else {
                    containmentView->destroy();
                }
            } else if (newScreen) {
                addOutput(newScreen);
            }

            if (contSwapView) {
                if (oldScreen) {
                    contSwapView->setScreenToFollow(oldScreen);
                    m_desktopViewForScreen[oldScreenId] = contSwapView;
                } else {
                    contSwapView->destroy();
                }
            } else if (oldScreen) {
                addOutput(oldScreen);
            }
        }
    }

    m_pendingScreenChanges.clear();
}

int ShellCorona::screenForContainment(const Plasma::Containment *containment) const
{
    // TODO: when we can depend on a new framework, use a p-f method to actuall set lastScreen instead of this?
    // m_pendingScreenChanges controls an explicit user-determined screen change
    if (!m_pendingScreenChanges.isEmpty() && m_pendingScreenChanges.contains(containment)) {
        return m_pendingScreenChanges.value(containment);
    }

    // case in which this containment is child of an applet, hello systray :)
    if (Plasma::Applet *parentApplet = qobject_cast<Plasma::Applet *>(containment->parent())) {
        if (Plasma::Containment *cont = parentApplet->containment()) {
            return screenForContainment(cont);
        } else {
            return -1;
        }
    }

    // This part is necessary as is the thing which promotes a containment from screen -1 to
    // its final screen which will be saved as lastScreen)
    // if the desktop views already exist, base the decision upon them
    for (auto it = m_desktopViewForScreen.constBegin(), end = m_desktopViewForScreen.constEnd(); it != end; ++it) {
        if (it.value()->containment() == containment && containment->activity() == m_activityController->currentActivity()) {
            return m_screenPool->idForScreen(it.value()->screenToFollow());
        }
    }

    // if the panel views already exist, base upon them
    PanelView *view = m_panelViews.value(containment);
    if (view && view->screenToFollow()) {
        return m_screenPool->idForScreen(view->screenToFollow());
    }

    return -1;
}

void ShellCorona::grabContainmentPreview(Plasma::Containment *containment)
{
    QQuickWindow *viewToGrab = nullptr;

    if (containment->containmentType() == Plasma::Containment::Panel || containment->containmentType() == Plasma::Containment::CustomPanel) {
        // Panel Containment
        auto it = m_panelViews.constBegin();
        while (it != m_panelViews.constEnd()) {
            if (it.key() == containment) {
                viewToGrab = it.value();
                break;
            }
            it++;
        }
    } else if (m_desktopViewForScreen.contains(containment->screen())) {
        // Desktop Containment
        viewToGrab = m_desktopViewForScreen[containment->screen()];
    }

    if (viewToGrab) {
        QSize size(512, 512);
        size = viewToGrab->size().scaled(size, Qt::KeepAspectRatio);
        auto result = viewToGrab->contentItem()->grabToImage(size);

        if (result) {
            connect(result.data(), &QQuickItemGrabResult::ready, this, [this, result, containment]() {
                // DataLocation is plasmashell, we need just "plasma"
                QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
                QDir dir(path);
                path += QStringLiteral("/plasma/containmentpreviews/");
                dir.mkpath(path);
                path += QString::number(containment->id()) + QChar('-') + containment->activity() + QStringLiteral(".png");
                result->saveToFile(path);
                emit containmentPreviewReady(containment, path);
            });
        }
    }
}

QString ShellCorona::containmentPreviewPath(Plasma::Containment *containment) const
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/plasma/containmentpreviews/")
        + QString::number(containment->id()) + QChar('-') + containment->activity() + QStringLiteral(".png");
    if (QFile::exists(path)) {
        return path;
    } else {
        return QString();
    }
}

bool ShellCorona::accentColorFromWallpaperEnabled() const
{
    return m_accentColorFromWallpaperEnabled;
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
    connect(registry, &Registry::plasmaShellAnnounced, this, [this, registry](quint32 name, quint32 version) {
        m_waylandPlasmaShell = registry->createPlasmaShell(name, version, this);
    });
    connect(registry, &KWayland::Client::Registry::plasmaWindowManagementAnnounced, this, [this, registry](quint32 name, quint32 version) {
        m_waylandWindowManagement = registry->createPlasmaWindowManagement(name, version, this);
    });
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
    QList<int> ids;
    for (int i = 0; i < m_screenPool->screenOrder().size(); ++i) {
        ids.append(i);
    }
    return ids;
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
    for (PanelView *view : qAsConst(m_panelViews)) {
        view->updateExclusiveZone();
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
    auto message = QDBusMessage::createMethodCall("org.kde.KWin", "/KWin", "org.kde.KWin", "activeOutputName");
    auto watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(message));
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, watcher] {
        watcher->deleteLater();
        QDBusReply<QString> reply = *watcher;
        if (reply.isValid()) {
            activateLauncherMenu(reply.value());
        }
    });
}

void ShellCorona::activateLauncherMenu(const QString &screenName)
{
    auto activateLauncher = [](Plasma::Applet *applet) -> bool {
        const auto provides = applet->pluginMetaData().value(QStringLiteral("X-Plasma-Provides"), QStringList());
        if (provides.contains(QLatin1String("org.kde.plasma.launchermenu"))) {
            Q_EMIT applet->activated();
            return true;
        }
        return false;
    };

    int screenId = m_screenPool->idForName(screenName);

    for (auto *cont : containments()) {
        if ((screenId < 0 || cont->screen() == screenId)
            && (cont->containmentType() == Plasma::Containment::Panel || cont->containmentType() == Plasma::Containment::CustomPanel)) {
            const auto applets = cont->applets();
            for (auto applet : applets) {
                if (activateLauncher(applet)) {
                    return;
                }
            }
            if (activateLauncher(cont)) {
                return;
            }
        }
    }

    for (auto *cont : containments()) {
        if ((screenId < 0 || cont->screen() == screenId) && cont->containmentType() == Plasma::Containment::Desktop) {
            const auto applets = cont->applets();
            for (auto applet : applets) {
                if (activateLauncher(applet)) {
                    return;
                }
            }
            if (activateLauncher(cont)) {
                return;
            }
        }
    }

    if (screenId >= 0) {
        activateLauncherMenu(QString());
    }
}

void ShellCorona::activateTaskManagerEntry(int index)
{
    auto activateTaskManagerEntryOnContainment = [](const Plasma::Containment *c, int index) {
        const auto &applets = c->applets();
        for (auto *applet : applets) {
            const auto &provides = applet->pluginMetaData().value(QStringLiteral("X-Plasma-Provides"), QStringList());
            if (provides.contains(QLatin1String("org.kde.plasma.multitasking"))) {
                if (QQuickItem *appletInterface = PlasmaQuick::AppletQuickItem::itemForApplet(applet)) {
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
        if (it.value()->screen() != m_screenPool->primaryScreen()) {
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

QString ShellCorona::defaultShell()
{
    KSharedConfig::Ptr startupConf = KSharedConfig::openConfig(QStringLiteral("plasmashellrc"));
    KConfigGroup startupConfGroup(startupConf, "Shell");
    const QString defaultValue = qEnvironmentVariable("PLASMA_DEFAULT_SHELL", "org.kde.plasma.desktop");
    QString value = startupConfGroup.readEntry("ShellPackage", defaultValue);

    // In the global theme an empty value was written, make sure we still return a shell package
    return value.isEmpty() ? defaultValue : value;
}

void ShellCorona::refreshCurrentShell()
{
    KSharedConfig::openConfig(QStringLiteral("plasmashellrc"))->reparseConfiguration();
    //  FIXME:   setShell(defaultShell());
    QProcess::startDetached("plasmashell", {"--replace"});
}

// Desktop corona handler

#include "moc_shellcorona.cpp"
#include "shellcorona.moc"
