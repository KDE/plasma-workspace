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
#include "systemtraymodel.h"
#include "sortedsystemtraymodel.h"
#include "debug.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusServiceWatcher>
#include <QMenu>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegExp>
#include <QScreen>
#include <QTimer>

#include <Plasma/PluginLoader>
#include <Plasma/ServiceJob>

#include <KActionCollection>
#include <KAcceleratorManager>
#include <KConfigLoader>
#include <KLocalizedString>

#include <plasma_version.h>

SystemTray::SystemTray(QObject *parent, const QVariantList &args)
    : Plasma::Containment(parent, args),
      m_systemTrayModel(nullptr),
      m_sortedSystemTrayModel(nullptr),
      m_configSystemTrayModel(nullptr),
      m_sessionServiceWatcher(new QDBusServiceWatcher(this)),
      m_systemServiceWatcher(new QDBusServiceWatcher(this))
{
    setHasConfigurationInterface(true);
    setContainmentType(Plasma::Types::CustomEmbeddedContainment);

    m_sessionServiceWatcher->setConnection(QDBusConnection::sessionBus());
    m_systemServiceWatcher->setConnection(QDBusConnection::systemBus());
}

SystemTray::~SystemTray()
{
}

void SystemTray::init()
{
    Containment::init();

    for (const auto &info: Plasma::PluginLoader::self()->listAppletMetaData(QString())) {
        if (!info.isValid() || info.value(QStringLiteral("X-Plasma-NotificationArea")) != "true") {
            continue;
        }
        m_systrayApplets[info.pluginId()] = info;

        if (info.isEnabledByDefault()) {
            m_defaultPlasmoids += info.pluginId();
        }
        const QString dbusactivation = info.value(QStringLiteral("X-Plasma-DBusActivationService"));
        if (!dbusactivation.isEmpty()) {
            qCDebug(SYSTEM_TRAY) << "ST Found DBus-able Applet: " << info.pluginId() << dbusactivation;
            QRegExp rx(dbusactivation);
            rx.setPatternSyntax(QRegExp::Wildcard);
            m_dbusActivatableTasks[info.pluginId()] = rx;

            const QString watchedService = QString(dbusactivation).replace(".*", "*");
            m_sessionServiceWatcher->addWatchedService(watchedService);
            m_systemServiceWatcher->addWatchedService(watchedService);
        }
    }
}

void SystemTray::newTask(const QString &task)
{
    const auto appletsList = applets();
    for (Plasma::Applet *applet : appletsList) {
        if (!applet->pluginMetaData().isValid()) {
            continue;
        }

        //only allow one instance per applet
        if (task == applet->pluginMetaData().pluginId()) {
            //Applet::destroy doesn't delete the applet from Containment::applets in the same event
            //potentially a dbus activated service being restarted can be added in this time.
            if (!applet->destroyed()) {
                return;
            }
        }
    }

    //known one, recycle the id to reuse old config
    if (m_knownPlugins.contains(task)) {
        Applet *applet = Plasma::PluginLoader::self()->loadApplet(task, m_knownPlugins.value(task), QVariantList());
        //this should never happen unless explicitly wrong config is hand-written or
        //(more likely) a previously added applet is uninstalled
        if (!applet) {
            qWarning() << "Unable to find applet" << task;
            return;
        }
        applet->setProperty("org.kde.plasma:force-create", true);
        addApplet(applet);
    //create a new one automatic id, new config group
    } else {
        Applet * applet = createApplet(task, QVariantList() << "org.kde.plasma:force-create");
        if (applet) {
            m_knownPlugins[task] = applet->id();
        }
    }
}

void SystemTray::cleanupTask(const QString &task)
{
    const auto appletsList = applets();
    for (Plasma::Applet *applet : appletsList) {
        if (applet->pluginMetaData().isValid() && task == applet->pluginMetaData().pluginId()) {
            //we are *not* cleaning the config here, because since is one
            //of those automatically loaded/unloaded by dbus, we want to recycle
            //the config the next time it's loaded, in case the user configured something here
            applet->deleteLater();
            //HACK: we need to remove the applet from Containment::applets() as soon as possible
            //otherwise we may have disappearing applets for restarting dbus services
            //this may be removed when we depend from a frameworks version in which appletDeleted is emitted as soon as deleteLater() is called
            emit appletDeleted(applet);
        }
    }
}

void SystemTray::showPlasmoidMenu(QQuickItem *appletInterface, int x, int y)
{
    if (!appletInterface) {
        return;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet*>();

    QPointF pos = appletInterface->mapToScene(QPointF(x, y));

    if (appletInterface->window() && appletInterface->window()->screen()) {
        pos = appletInterface->window()->mapToGlobal(pos.toPoint());
    } else {
        pos = QPoint();
    }

    QMenu *desktopMenu = new QMenu;
    connect(this, &QObject::destroyed, desktopMenu, &QMenu::close);
    desktopMenu->setAttribute(Qt::WA_DeleteOnClose);

    //this is a workaround where Qt will fail to realize a mouse has been released

    // this happens if a window which does not accept focus spawns a new window that takes focus and X grab
    // whilst the mouse is depressed
    // https://bugreports.qt.io/browse/QTBUG-59044
    // this causes the next click to go missing

    //by releasing manually we avoid that situation
    auto ungrabMouseHack = [appletInterface]() {
        if (appletInterface->window() && appletInterface->window()->mouseGrabberItem()) {
            appletInterface->window()->mouseGrabberItem()->ungrabMouse();
        }
    };

    QTimer::singleShot(0, appletInterface, ungrabMouseHack);
    //end workaround


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

        pos = QPoint(qBound(geo.left(), (int)pos.x(), geo.right() - desktopMenu->width()),
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

    if (menu) {
        menu->adjustSize();
        const auto parameters = sjob->parameters();
        int x = parameters[QStringLiteral("x")].toInt();
        int y = parameters[QStringLiteral("y")].toInt();

        //try tofind the icon screen coordinates, and adjust the position as a poor
        //man's popupPosition

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

QPointF SystemTray::popupPosition(QQuickItem* visualParent, int x, int y)
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
    return m_systrayApplets.contains(appletId);
}

Plasma::Service *SystemTray::serviceForSource(const QString &source)
{
    return m_statusNotifierModel->serviceForSource(source);
}

void SystemTray::restoreContents(KConfigGroup &group)
{
    QStringList newKnownItems;
    QStringList newExtraItems;

    KConfigLoader *scheme = configScheme();
    KConfigGroup general = group.group("General");

    QStringList knownItems = general.readEntry("knownItems", scheme->property("knownItems").toStringList());
    QStringList extraItems = general.readEntry("extraItems", scheme->property("extraItems").toStringList());

    //Add every plasmoid that is both not enabled explicitly and not already known
    for (int i = 0; i < m_defaultPlasmoids.length(); ++i) {
        QString candidate = m_defaultPlasmoids[i];
        if (!knownItems.contains(candidate)) {
            newKnownItems.append(candidate);
            if (!extraItems.contains(candidate)) {
                newExtraItems.append(candidate);
            }
        }
    }

    if (newExtraItems.length() > 0) {
        general.writeEntry("extraItems", extraItems + newExtraItems);
    }
    if (newKnownItems.length() > 0) {
        general.writeEntry("knownItems", knownItems + newKnownItems);
    }
    // refresh state of config scheme, if not, above writes are ignored
    scheme->read();

    setAllowedPlasmoids(general.readEntry("extraItems", scheme->property("extraItems").toStringList()));

    emit configurationChanged(config());
}

void SystemTray::restorePlasmoids()
{
    if (!isContainment()) {
        qCWarning(SYSTEM_TRAY) << "Loaded as an applet, this shouldn't have happened";
        return;
    }

    //First: remove all that are not allowed anymore
    const auto appletsList = applets();
    for (Plasma::Applet *applet : appletsList) {
        //Here it should always be valid.
        //for some reason it not always is.
        if (!applet->pluginMetaData().isValid()) {
            applet->config().parent().deleteGroup();
            applet->deleteLater();
        } else {
            const QString task = applet->pluginMetaData().pluginId();
            if (!m_allowedPlasmoids.contains(task)) {
                //in those cases we do delete the applet config completely
                //as they were explicitly disabled by the user
                applet->config().parent().deleteGroup();
                applet->deleteLater();
            }
        }
    }

    KConfigGroup cg = config();

    cg = KConfigGroup(&cg, "Applets");

    const auto groups = cg.groupList();
    for (const QString &group : groups) {
        KConfigGroup appletConfig(&cg, group);
        QString plugin = appletConfig.readEntry("plugin");
        if (!plugin.isEmpty()) {
            m_knownPlugins[plugin] = group.toInt();
        }
    }

    QMap<QString, KPluginMetaData> sortedApplets;
    for (auto it = m_systrayApplets.constBegin(); it != m_systrayApplets.constEnd(); ++it) {
        const KPluginMetaData &info = it.value();
        if (m_allowedPlasmoids.contains(info.pluginId()) && !
            m_dbusActivatableTasks.contains(info.pluginId())) {
            //FIXME
            // if we already have a plugin with this exact name in it, then check if it is the
            // same plugin and skip it if it is indeed already listed
            if (sortedApplets.contains(info.name())) {
                bool dupe = false;
                // it is possible (though poor form) to have multiple applets
                // with the same visible name but different plugins, so we hve to check all values
                const auto infos = sortedApplets.values(info.name());
                for (const KPluginMetaData &existingInfo : infos) {
                    if (existingInfo.pluginId() == info.pluginId()) {
                        dupe = true;
                        break;
                    }
                }

                if (dupe) {
                    continue;
                }
            }

            // insertMulti because it is possible (though poor form) to have multiple applets
            // with the same visible name but different plugins
            sortedApplets.insertMulti(info.name(), info);
        }
    }

    for (const KPluginMetaData &info : qAsConst(sortedApplets)) {
        qCDebug(SYSTEM_TRAY) << " Adding applet: " << info.name();
        if (m_allowedPlasmoids.contains(info.pluginId())) {
            newTask(info.pluginId());
        }
    }

    initDBusActivatables();
}

void SystemTray::configChanged()
{
    Containment::configChanged();
    emit configurationChanged(config());
}

SystemTrayModel *SystemTray::systemTrayModel()
{
    if (!m_systemTrayModel) {
        m_systemTrayModel = new SystemTrayModel(this);

        PlasmoidModel *currentPlasmoidsModel = new PlasmoidModel(m_systemTrayModel);
        connect(this, &SystemTray::appletAdded, currentPlasmoidsModel, &PlasmoidModel::addApplet);
        connect(this, &SystemTray::appletRemoved, currentPlasmoidsModel, &PlasmoidModel::removeApplet);
        connect(this, &SystemTray::configurationChanged, currentPlasmoidsModel, &PlasmoidModel::onConfigurationChanged);
        currentPlasmoidsModel->onConfigurationChanged(this->config());
        for (auto applet : applets()) {
            currentPlasmoidsModel->addApplet(applet);
        }

        m_statusNotifierModel = new StatusNotifierModel(m_systemTrayModel);
        connect(this, &SystemTray::configurationChanged, m_statusNotifierModel, &StatusNotifierModel::onConfigurationChanged);
        m_statusNotifierModel->onConfigurationChanged(this->config());

        m_systemTrayModel->addSourceModel(currentPlasmoidsModel);
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

QStringList SystemTray::allowedPlasmoids() const
{
    return m_allowedPlasmoids;
}

void SystemTray::setAllowedPlasmoids(const QStringList &allowed)
{
    if (allowed == m_allowedPlasmoids) {
        return;
    }

    m_allowedPlasmoids = allowed;

    restorePlasmoids();
    emit allowedPlasmoidsChanged();
}

/* Loading and unloading Plasmoids when dbus services come and go
 *
 * This works as follows:
 * - we collect a list of plugins and related services in m_dbusActivatableTasks
 * - we query DBus for the list of services, async (initDBusActivatables())
 * - we go over that list, adding tasks when a service and plugin match (serviceNameFetchFinished())
 * - we start watching for new services, and do the same (serviceNameFetchFinished())
 * - whenever a service is gone, we check whether to unload a Plasmoid (serviceUnregistered())
 *
 * Order of events has to be:
 * - create a match rule for new service on DBus daemon
 * - start fetching a list of names
 * - ignore all changes that happen in the meantime
 * - handle the list of all names
 */
void SystemTray::initDBusActivatables()
{
    // Watch for new services
    connect(m_sessionServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &serviceName)
    {
        if (!m_dbusSessionServiceNamesFetched) {
            return;
        }
        serviceRegistered(serviceName);
    });
    connect(m_sessionServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName)
    {
        if (!m_dbusSessionServiceNamesFetched) {
            return;
        }
        serviceUnregistered(serviceName);
    });
    connect(m_systemServiceWatcher, &QDBusServiceWatcher::serviceRegistered, this, [this](const QString &serviceName)
    {
        if (!m_dbusSystemServiceNamesFetched) {
            return;
        }
        serviceRegistered(serviceName);
    });
    connect(m_systemServiceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName)
    {
        if (!m_dbusSystemServiceNamesFetched) {
            return;
        }
        serviceUnregistered(serviceName);
    });

    // fetch list of existing services
    QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, [=](QDBusPendingCallWatcher *callWatcher) {
        serviceNameFetchFinished(callWatcher);
        m_dbusSessionServiceNamesFetched = true;
    });

    QDBusPendingCall systemAsync = QDBusConnection::systemBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *systemCallWatcher = new QDBusPendingCallWatcher(systemAsync, this);
    connect(systemCallWatcher, &QDBusPendingCallWatcher::finished, [=](QDBusPendingCallWatcher *callWatcher) {
        serviceNameFetchFinished(callWatcher);
        m_dbusSystemServiceNamesFetched = true;
    });
}

void SystemTray::serviceNameFetchFinished(QDBusPendingCallWatcher* watcher)
{
    QDBusPendingReply<QStringList> propsReply = *watcher;
    watcher->deleteLater();

    if (propsReply.isError()) {
        qCWarning(SYSTEM_TRAY) << "Could not get list of available D-Bus services";
    } else {
        const auto propsReplyValue = propsReply.value();
        for (const QString& serviceName : propsReplyValue) {
            serviceRegistered(serviceName);
        }
    }
}

void SystemTray::serviceRegistered(const QString &service)
{
    if (service.startsWith(QLatin1Char(':'))) {
        return;
    }

    //qCDebug(SYSTEM_TRAY) << "DBus service appeared:" << service;
    for (auto it = m_dbusActivatableTasks.constBegin(), end = m_dbusActivatableTasks.constEnd(); it != end; ++it) {
        const QString &plugin = it.key();
        if (!m_allowedPlasmoids.contains(plugin)) {
            continue;
        }

        const auto &rx = it.value();
        if (rx.exactMatch(service)) {
            //qCDebug(SYSTEM_TRAY) << "ST : DBus service " << m_dbusActivatableTasks[plugin] << "appeared. Loading " << plugin;
            newTask(plugin);
            m_dbusServiceCounts[plugin]++;
        }
    }
}

void SystemTray::serviceUnregistered(const QString &service)
{
    //qCDebug(SYSTEM_TRAY) << "DBus service disappeared:" << service;

    for (auto it = m_dbusActivatableTasks.constBegin(), end = m_dbusActivatableTasks.constEnd(); it != end; ++it) {
        const QString &plugin = it.key();
        if (!m_allowedPlasmoids.contains(plugin)) {
            continue;
        }

        const auto &rx = it.value();
        if (rx.exactMatch(service)) {
            m_dbusServiceCounts[plugin]--;
            Q_ASSERT(m_dbusServiceCounts[plugin] >= 0);
            if (m_dbusServiceCounts[plugin] == 0) {
                //qCDebug(SYSTEM_TRAY) << "ST : DBus service " << m_dbusActivatableTasks[plugin] << " disappeared. Unloading " << plugin;
                cleanupTask(plugin);
            }
        }
    }
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemtray, SystemTray, "metadata.json")

#include "systemtray.moc"
