/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "systemtray.h"
#include "debug.h"

#include "plasmoidregistry.h"
#include "sortedsystemtraymodel.h"
#include "systemtraymodel.h"
#include "systemtraysettings.h"

#include <QMenu>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

#include <Plasma/Applet>
#include <Plasma/PluginLoader>
#include <Plasma/ServiceJob>

#include <KAcceleratorManager>
#include <KActionCollection>

SystemTray::SystemTray(QObject *parent, const QVariantList &args)
    : Plasma::Containment(parent, args)
    , m_plasmoidModel(nullptr)
    , m_statusNotifierModel(nullptr)
    , m_systemTrayModel(nullptr)
    , m_sortedSystemTrayModel(nullptr)
    , m_configSystemTrayModel(nullptr)
{
    setHasConfigurationInterface(true);
    setContainmentType(Plasma::Types::CustomEmbeddedContainment);
    setContainmentDisplayHints(Plasma::Types::ContainmentDrawsPlasmoidHeading | Plasma::Types::ContainmentForcesSquarePlasmoids);
}

SystemTray::~SystemTray()
{
}

void SystemTray::init()
{
    Containment::init();

    m_settings = new SystemTraySettings(configScheme(), this);
    connect(m_settings, &SystemTraySettings::enabledPluginsChanged, this, &SystemTray::onEnabledAppletsChanged);

    m_plasmoidRegistry = new PlasmoidRegistry(m_settings, this);
    connect(m_plasmoidRegistry, &PlasmoidRegistry::plasmoidEnabled, this, &SystemTray::startApplet);
    connect(m_plasmoidRegistry, &PlasmoidRegistry::plasmoidStopped, this, &SystemTray::stopApplet);

    // we don't want to automatically propagate the activated signal from the Applet to the Containment
    // even if SystemTray is of type Containment, it is de facto Applet and should act like one
    connect(this, &Containment::appletAdded, this, [this](Plasma::Applet *applet) {
        disconnect(applet, &Applet::activated, this, &Applet::activated);
    });
}

void SystemTray::restoreContents(KConfigGroup &group)
{
    if (!isContainment()) {
        qCWarning(SYSTEM_TRAY) << "Loaded as an applet, this shouldn't have happened";
        return;
    }

    KConfigGroup shortcutConfig(&group, "Shortcuts");
    QString shortcutText = shortcutConfig.readEntryUntranslated("global", QString());
    if (!shortcutText.isEmpty()) {
        setGlobalShortcut(QKeySequence(shortcutText));
    }

    // cache known config group ids for applets
    KConfigGroup cg = group.group("Applets");
    for (const QString &group : cg.groupList()) {
        KConfigGroup appletConfig(&cg, group);
        QString plugin = appletConfig.readEntry("plugin");
        if (!plugin.isEmpty()) {
            m_configGroupIds[plugin] = group.toInt();
        }
    }

    m_plasmoidRegistry->init();
}

void SystemTray::showPlasmoidMenu(QQuickItem *appletInterface, int x, int y)
{
    if (!appletInterface) {
        return;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet *>();

    QPointF pos = appletInterface->mapToScene(QPointF(x, y));

    if (appletInterface->window() && appletInterface->window()->screen()) {
        pos = appletInterface->window()->mapToGlobal(pos.toPoint());
    } else {
        pos = QPoint();
    }

    QMenu *desktopMenu = new QMenu;
    connect(this, &QObject::destroyed, desktopMenu, &QMenu::close);
    desktopMenu->setAttribute(Qt::WA_DeleteOnClose);

    // this is a workaround where Qt will fail to realize a mouse has been released

    // this happens if a window which does not accept focus spawns a new window that takes focus and X grab
    // whilst the mouse is depressed
    // https://bugreports.qt.io/browse/QTBUG-59044
    // this causes the next click to go missing

    // by releasing manually we avoid that situation
    auto ungrabMouseHack = [appletInterface]() {
        if (appletInterface->window() && appletInterface->window()->mouseGrabberItem()) {
            appletInterface->window()->mouseGrabberItem()->ungrabMouse();
        }
    };

    QTimer::singleShot(0, appletInterface, ungrabMouseHack);
    // end workaround

    emit applet->contextualActionsAboutToShow();
    const auto contextActions = applet->contextualActions();
    for (QAction *action : contextActions) {
        if (action) {
            desktopMenu->addAction(action);
        }
    }

    QAction *runAssociatedApplication = applet->actions()->action(QStringLiteral("run associated application"));
    if (runAssociatedApplication && runAssociatedApplication->isEnabled()) {
        desktopMenu->addAction(runAssociatedApplication);
    }

    if (applet->actions()->action(QStringLiteral("configure"))) {
        desktopMenu->addAction(applet->actions()->action(QStringLiteral("configure")));
    }

    if (desktopMenu->isEmpty()) {
        delete desktopMenu;
        return;
    }

    desktopMenu->adjustSize();

    if (QScreen *screen = appletInterface->window()->screen()) {
        const QRect geo = screen->availableGeometry();

        pos = QPoint(qBound(geo.left(), (int)pos.x(), geo.right() - desktopMenu->width()), //
                     qBound(geo.top(), (int)pos.y(), geo.bottom() - desktopMenu->height()));
    }

    KAcceleratorManager::manage(desktopMenu);
    desktopMenu->winId();
    desktopMenu->windowHandle()->setTransientParent(appletInterface->window());
    desktopMenu->popup(pos.toPoint());
}

void SystemTray::showStatusNotifierContextMenu(KJob *job, QQuickItem *statusNotifierIcon)
{
    if (QCoreApplication::closingDown() || !statusNotifierIcon) {
        // apparently an edge case can be triggered due to the async nature of all this
        // see: https://bugs.kde.org/show_bug.cgi?id=251977
        return;
    }

    Plasma::ServiceJob *sjob = qobject_cast<Plasma::ServiceJob *>(job);
    if (!sjob) {
        return;
    }

    QMenu *menu = qobject_cast<QMenu *>(sjob->result().value<QObject *>());

    if (menu && !menu->isEmpty()) {
        menu->adjustSize();
        const auto parameters = sjob->parameters();
        int x = parameters[QStringLiteral("x")].toInt();
        int y = parameters[QStringLiteral("y")].toInt();

        // try tofind the icon screen coordinates, and adjust the position as a poor
        // man's popupPosition

        QRect screenItemRect(statusNotifierIcon->mapToScene(QPointF(0, 0)).toPoint(), QSize(statusNotifierIcon->width(), statusNotifierIcon->height()));

        if (statusNotifierIcon->window()) {
            screenItemRect.moveTopLeft(statusNotifierIcon->window()->mapToGlobal(screenItemRect.topLeft()));
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
            if (screenItemRect.top() - menu->height() >= statusNotifierIcon->window()->screen()->geometry().top()) {
                y = screenItemRect.top() - menu->height();
            } else {
                y = screenItemRect.bottom();
            }
        }

        KAcceleratorManager::manage(menu);
        menu->winId();
        menu->windowHandle()->setTransientParent(statusNotifierIcon->window());
        menu->popup(QPoint(x, y));
    }
}

QPointF SystemTray::popupPosition(QQuickItem *visualParent, int x, int y)
{
    if (!visualParent) {
        return QPointF(0, 0);
    }

    QPointF pos = visualParent->mapToScene(QPointF(x, y));

    if (visualParent->window() && visualParent->window()->screen()) {
        pos = visualParent->window()->mapToGlobal(pos.toPoint());
    } else {
        return QPoint();
    }
    return pos;
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
            applet->deleteLater();
        } else {
            const QString task = applet->pluginMetaData().pluginId();
            if (!m_settings->isEnabledPlugin(task)) {
                // in those cases we do delete the applet config completely
                // as they were explicitly disabled by the user
                applet->config().parent().deleteGroup();
                applet->deleteLater();
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
            qWarning() << "Unable to find applet" << pluginId;
            return;
        }
        applet->setProperty("org.kde.plasma:force-create", true);
        addApplet(applet);
        // create a new one automatic id, new config group
    } else {
        Applet *applet = createApplet(pluginId, QVariantList() << "org.kde.plasma:force-create");
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
            // we are *not* cleaning the config here, because since is one
            // of those automatically loaded/unloaded by dbus, we want to recycle
            // the config the next time it's loaded, in case the user configured something here
            applet->deleteLater();
            // HACK: we need to remove the applet from Containment::applets() as soon as possible
            // otherwise we may have disappearing applets for restarting dbus services
            // this may be removed when we depend from a frameworks version in which appletDeleted is emitted as soon as deleteLater() is called
            emit appletDeleted(applet);
        }
    }
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemtray, SystemTray, "metadata.json")

#include "systemtray.moc"
