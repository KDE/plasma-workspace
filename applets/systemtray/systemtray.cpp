/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <optional>

#include "config-X11.h"
#include "debug.h"
#include "systemtray.h"

#include "plasmoidregistry.h"
#include "sortedsystemtraymodel.h"
#include "systemtraymodel.h"
#include "systemtraysettings.h"

#include <QGuiApplication>
#include <QMenu>
#include <QMetaMethod>
#include <QMetaObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>
#include <qpa/qplatformscreen.h>

#include <Plasma/Applet>
#include <Plasma/PluginLoader>
#include <Plasma5Support/ServiceJob>

#include <KAcceleratorManager>
#include <KActionCollection>
#include <KSharedConfig>
#include <KWindowSystem>

using namespace Qt::StringLiterals;

SystemTray::SystemTray(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : Plasma::Containment(parent, data, args)
{
    setHasConfigurationInterface(true);
    setContainmentDisplayHints(Plasma::Types::ContainmentDrawsPlasmoidHeading | Plasma::Types::ContainmentForcesSquarePlasmoids);
}

SystemTray::~SystemTray()
{
    // When the applet is about to be deleted, delete now to avoid calling loadConfig()
    delete m_settings;
}

void SystemTray::init()
{
    Containment::init();

    m_settings = new SystemTraySettings(configScheme(), this);
    connect(m_settings, &SystemTraySettings::enabledPluginsChanged, this, &SystemTray::onEnabledAppletsChanged);

    m_plasmoidRegistry = new PlasmoidRegistry(m_settings, this);
    connect(m_plasmoidRegistry, &PlasmoidRegistry::plasmoidEnabled, this, &SystemTray::startApplet);
    connect(m_plasmoidRegistry, &PlasmoidRegistry::plasmoidStopped, this, &SystemTray::stopApplet);
    connect(
        this,
        &Containment::appletRemoved,
        this,
        [&](auto applet) {
            // First of all, let's immediately add the applet back. If we allow the user to entirely
            // remove the applet from the system tray, they won't have any way to add it back (it won't
            // even be on the list of plugins to enable in tray settings.)
            m_settings->addEnabledPlugin(applet->pluginName());

            // Then, let's "cleanup" the applet, i.e. remove it from the list of shown/hidden applets.
            // This will leave the applet within the system tray, but it will be marked as disabled.
            m_settings->cleanupPlugin(applet->pluginName());
        },
        Qt::SingleShotConnection); // Make sure to use SingleShotConnection so we don't end in recursive loop

    // we don't want to automatically propagate the activated signal from the Applet to the Containment
    // even if SystemTray is of type Containment, it is de facto Applet and should act like one
    connect(this, &Containment::appletAdded, this, [this](Plasma::Applet *applet) {
        disconnect(applet, &Applet::activated, this, &Applet::activated);
    });

    if (KWindowSystem::isPlatformWayland()) {
        auto config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"), KConfig::NoGlobals);
        KConfigGroup kscreenGroup = config->group(QStringLiteral("KScreen"));
        m_xwaylandClientsScale = kscreenGroup.readEntry("XwaylandClientsScale", true);

        m_configWatcher = KConfigWatcher::create(config);
        connect(m_configWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
            if (group.name() == QStringLiteral("KScreen") && names.contains(QByteArrayLiteral("XwaylandClientsScale"))) {
                m_xwaylandClientsScale = group.readEntry("XwaylandClientsScale", true);
            }
        });
    }
}

void SystemTray::restoreContents(KConfigGroup &group)
{
    if (!isContainment()) {
        qCWarning(SYSTEM_TRAY) << "Loaded as an applet, this shouldn't have happened";
        return;
    }

    KConfigGroup shortcutConfig(&group, u"Shortcuts"_s);
    QString shortcutText = shortcutConfig.readEntryUntranslated("global", QString());
    if (!shortcutText.isEmpty()) {
        setGlobalShortcut(QKeySequence(shortcutText));
    }

    // cache known config group ids for applets
    KConfigGroup cg = group.group(u"Applets"_s);
    for (const QString &group : cg.groupList()) {
        KConfigGroup appletConfig(&cg, group);
        QString plugin = appletConfig.readEntry("plugin");
        if (!plugin.isEmpty()) {
            m_configGroupIds[plugin] = group.toInt();
        }
    }

    m_plasmoidRegistry->init();
}

void SystemTray::showPlasmoidMenu(QQuickItem *appletInterface)
{
    if (!appletInterface) {
        return;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet *>();

    QMenu *desktopMenu = new QMenu;
    connect(this, &QObject::destroyed, desktopMenu, &QMenu::close);
    desktopMenu->setAttribute(Qt::WA_DeleteOnClose);

    Q_EMIT applet->contextualActionsAboutToShow();
    const auto contextActions = applet->contextualActions();
    for (QAction *action : contextActions) {
        if (action) {
            desktopMenu->addAction(action);
        }
    }

    if (applet->internalAction(QStringLiteral("configure"))) {
        desktopMenu->addAction(applet->internalAction(QStringLiteral("configure")));
    }

    if (desktopMenu->isEmpty()) {
        delete desktopMenu;
        return;
    }

    showContextMenu(appletInterface, desktopMenu);
}

void SystemTray::showStatusNotifierContextMenu(KJob *job, QQuickItem *statusNotifierIcon)
{
    if (QCoreApplication::closingDown() || !statusNotifierIcon) {
        // apparently an edge case can be triggered due to the async nature of all this
        // see: https://bugs.kde.org/show_bug.cgi?id=251977
        return;
    }

    Plasma5Support::ServiceJob *sjob = qobject_cast<Plasma5Support::ServiceJob *>(job);
    if (!sjob) {
        return;
    }

    QMenu *menu = qobject_cast<QMenu *>(sjob->result().value<QObject *>());

    if (menu && !menu->isEmpty()) {
        showContextMenu(statusNotifierIcon, menu);
    }
}

void SystemTray::showContextMenu(QQuickItem *iconItem, QMenu *menu)
{
    // this is a workaround where Qt will fail to realize a mouse has been released

    // this happens if a window which does not accept focus spawns a new window that takes focus and X grab
    // whilst the mouse is depressed
    // https://bugreports.qt.io/browse/QTBUG-59044
    // this causes the next click to go missing

    // by releasing manually we avoid that situation
    auto ungrabMouseHack = [iconItem]() {
        if (iconItem->window() && iconItem->window()->mouseGrabberItem()) {
            iconItem->window()->mouseGrabberItem()->ungrabMouse();
        }
    };

    QTimer::singleShot(0, iconItem, ungrabMouseHack);
    // end workaround

    menu->adjustSize();

    // try to find the icon screen coordinates, and adjust the position as a poor
    // man's popupPosition

    int x = 0;
    int y = 0;

    QRect screenItemRect(iconItem->mapToScene(QPointF(0, 0)).toPoint(), QSize(iconItem->width(), iconItem->height()));

    if (iconItem->window()) {
        screenItemRect.moveTopLeft(iconItem->window()->mapToGlobal(screenItemRect.topLeft()));
    }

    switch (location()) {
    case Plasma::Types::LeftEdge:
        x = screenItemRect.right();
        y = screenItemRect.top();
        break;
    case Plasma::Types::RightEdge:
        x = screenItemRect.left() - menu->width();
        y = screenItemRect.top();
        break;
    case Plasma::Types::TopEdge:
        x = screenItemRect.left();
        y = screenItemRect.bottom();
        break;
    case Plasma::Types::BottomEdge:
        x = screenItemRect.left();
        y = screenItemRect.top() - menu->height();
        break;
    default:
        x = screenItemRect.left();
        if (screenItemRect.top() - menu->height() >= iconItem->window()->screen()->geometry().top()) {
            y = screenItemRect.top() - menu->height();
        } else {
            y = screenItemRect.bottom();
        }
    }

    if (QScreen *screen = iconItem->window()->screen()) {
        const QRect geo = screen->availableGeometry();
        x = qBound(geo.left(), x, geo.right() - menu->width());
        y = qBound(geo.top(), y, geo.bottom() - menu->height());
    }

    KAcceleratorManager::manage(menu);
    menu->winId();
    menu->windowHandle()->setTransientParent(iconItem->window());
    menu->popup(QPoint(x, y));
}

QPointF SystemTray::popupPosition(QQuickItem *visualParent, int x, int y)
{
    if (!visualParent) {
        return QPointF(0, 0);
    }

    QPointF pos = visualParent->mapToScene(QPointF(x, y));

    QQuickWindow *const window = visualParent->window();
    if (window && window->screen()) {
        pos = window->mapToGlobal(pos.toPoint());
#if HAVE_X11
        if (KWindowSystem::isPlatformX11()) {
            const auto devicePixelRatio = window->screen()->devicePixelRatio();
            if (QGuiApplication::screens().size() == 1) {
                return pos * devicePixelRatio;
            }

            const QRect geometry = window->screen()->geometry();
            const QRect nativeGeometry = window->screen()->handle()->geometry();
            const QPointF nativeGlobalPosOnCurrentScreen = (pos - geometry.topLeft()) * devicePixelRatio;

            return nativeGeometry.topLeft() + nativeGlobalPosOnCurrentScreen;
        }
#endif

        if (KWindowSystem::isPlatformWayland()) {
            if (!m_xwaylandClientsScale) {
                return pos;
            }

            const qreal devicePixelRatio = window->devicePixelRatio();

            if (QGuiApplication::screens().size() == 1) {
                return pos * devicePixelRatio;
            }

            const QRect geometry = window->screen()->geometry();
            const QRect nativeGeometry = window->screen()->handle()->geometry();
            const QPointF nativeGlobalPosOnCurrentScreen = (pos - geometry.topLeft()) * devicePixelRatio;

            return nativeGeometry.topLeft() + nativeGlobalPosOnCurrentScreen;
        }
    }

    return QPoint();
}

bool SystemTray::isSystemTrayApplet(const QString &appletId)
{
    if (m_plasmoidRegistry) {
        return m_plasmoidRegistry->isSystemTrayApplet(appletId);
    }
    return false;
}

SystemTrayModel *SystemTray::systemTrayModel()
{
    if (!m_systemTrayModel) {
        m_systemTrayModel = new SystemTrayModel(this);

        m_plasmoidModel = new PlasmoidModel(m_settings, m_plasmoidRegistry, m_systemTrayModel);
        connect(this, &SystemTray::appletAdded, m_plasmoidModel, &PlasmoidModel::addApplet);
        connect(this, &SystemTray::appletRemoved, m_plasmoidModel, &PlasmoidModel::removeApplet);
        for (auto applet : applets()) {
            m_plasmoidModel->addApplet(applet);
        }

        m_statusNotifierModel = new StatusNotifierModel(m_settings, m_systemTrayModel);

        m_systemTrayModel->addSourceModel(m_plasmoidModel);
        m_systemTrayModel->addSourceModel(m_statusNotifierModel);
    }

    return m_systemTrayModel;
}

QAbstractItemModel *SystemTray::sortedSystemTrayModel()
{
    if (!m_sortedSystemTrayModel) {
        m_sortedSystemTrayModel = new SortedSystemTrayModel(SortedSystemTrayModel::SortingType::SystemTray, this);
        m_sortedSystemTrayModel->setSourceModel(systemTrayModel());
    }
    return m_sortedSystemTrayModel;
}

QAbstractItemModel *SystemTray::configSystemTrayModel()
{
    if (!m_configSystemTrayModel) {
        m_configSystemTrayModel = new SortedSystemTrayModel(SortedSystemTrayModel::SortingType::ConfigurationPage, this);
        m_configSystemTrayModel->setSourceModel(systemTrayModel());
    }
    return m_configSystemTrayModel;
}

void SystemTray::onEnabledAppletsChanged()
{
    // remove all that are not allowed anymore
    const auto appletsList = applets();
    for (Plasma::Applet *applet : appletsList) {
        // Here it should always be valid.
        // for some reason it not always is.
        if (!applet->pluginMetaData().isValid()) {
            applet->config().parent().deleteGroup();
            delete applet;
        } else {
            const QString task = applet->pluginMetaData().pluginId();
            if (!m_settings->isEnabledPlugin(task)) {
                // in those cases we do delete the applet config completely
                // as they were explicitly disabled by the user
                applet->config().parent().deleteGroup();
                delete applet;
                m_configGroupIds.remove(task);
            }
        }
    }
}

void SystemTray::startApplet(const QString &pluginId)
{
    const auto appletsList = applets();
    for (Plasma::Applet *applet : appletsList) {
        if (!applet->pluginMetaData().isValid()) {
            continue;
        }

        // only allow one instance per applet
        if (pluginId == applet->pluginMetaData().pluginId()) {
            // Applet::destroy doesn't delete the applet from Containment::applets in the same event
            // potentially a dbus activated service being restarted can be added in this time.
            if (!applet->destroyed()) {
                return;
            }
        }
    }

    qCDebug(SYSTEM_TRAY) << "Adding applet:" << pluginId;

    // known one, recycle the id to reuse old config
    if (m_configGroupIds.contains(pluginId)) {
        Applet *applet = Plasma::PluginLoader::self()->loadApplet(pluginId, m_configGroupIds.value(pluginId), QVariantList());
        // this should never happen unless explicitly wrong config is hand-written or
        //(more likely) a previously added applet is uninstalled
        if (!applet) {
            qCWarning(SYSTEM_TRAY) << "Unable to find applet" << pluginId;
            return;
        }
        applet->setProperty("org.kde.plasma:force-create", true);
        addApplet(applet);
        // create a new one automatic id, new config group
    } else {
        Applet *applet = createApplet(pluginId, QVariantList() << u"org.kde.plasma:force-create"_s);
        if (applet) {
            m_configGroupIds[pluginId] = applet->id();
        }
    }
}

void SystemTray::stopApplet(const QString &pluginId)
{
    const auto appletsList = applets();
    for (Plasma::Applet *applet : appletsList) {
        if (applet->pluginMetaData().isValid() && pluginId == applet->pluginMetaData().pluginId()) {
            delete applet;
        }
    }
}

void SystemTray::stackItemBefore(QQuickItem *newItem, QQuickItem *beforeItem)
{
    if (!newItem || !beforeItem) {
        return;
    }
    newItem->stackBefore(beforeItem);
}

void SystemTray::stackItemAfter(QQuickItem *newItem, QQuickItem *afterItem)
{
    if (!newItem || !afterItem) {
        return;
    }
    newItem->stackAfter(afterItem);
}

K_PLUGIN_CLASS(SystemTray)

#include "systemtray.moc"
