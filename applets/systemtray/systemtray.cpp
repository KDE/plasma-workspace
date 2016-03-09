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

#include <QDebug>
#include <QProcess>


#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusPendingCallWatcher>
#include <QMenu>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegExp>
#include <QScreen>
#include <QStandardItemModel>

#include <Plasma/PluginLoader>
#include <Plasma/ServiceJob>

#include <KIconLoader>
#include <KIconEngine>
#include <KActionCollection>
#include <KLocalizedString>

Q_LOGGING_CATEGORY(SYSTEMTRAY, "systemtray")

class PlasmoidModel: public QStandardItemModel
{
public:
    PlasmoidModel(QObject *parent = 0)
        : QStandardItemModel(parent)
    {
    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles = QStandardItemModel::roleNames();
        roles[Qt::UserRole+1] = "plugin";
        return roles;
    }
};

SystemTray::SystemTray(QObject *parent, const QVariantList &args)
    : Plasma::Containment(parent, args),
      m_availablePlasmoidsModel(nullptr)
{
    setHasConfigurationInterface(true);
    setContainmentType(Plasma::Types::CustomPanelContainment);
}

SystemTray::~SystemTray()
{
}

void SystemTray::init()
{
    config().writeEntry("lastScreen", -1);
    Containment::init();
    //actions()->removeAction(actions()->action("add widgets"));
    //actions()->removeAction(actions()->action("add panel"));
    //actions()->removeAction(actions()->action("lock widgets"));
}

void SystemTray::newTask(const QString &task)
{
    foreach (Plasma::Applet *applet, applets()) {
        if (!applet->pluginInfo().isValid()) {
            continue;
        }

        //only allow one instance per applet
        if (task == applet->pluginInfo().pluginName()) {
            return;
        }
    }

    createApplet(task);
}

void SystemTray::cleanupTask(const QString &task)
{
    foreach (Plasma::Applet *applet, applets()) {
        if (!applet->pluginInfo().isValid() || task == applet->pluginInfo().pluginName()) {
            applet->destroy();
        }
    }
}

QVariant SystemTray::resolveIcon(const QVariant &variant, const QString &iconThemePath)
{
    if (variant.canConvert<QString>()) {
        if (!iconThemePath.isEmpty()) {
            const QString path = iconThemePath;
            if (!path.isEmpty()) {
                // FIXME: If last part of path is not "icons", this won't work!
                QStringList tokens = path.split('/', QString::SkipEmptyParts);
                if (tokens.length() >= 3 && tokens.takeLast() == QLatin1String("icons")) {
                    QString appName = tokens.takeLast();

                    // We use a separate instance of KIconLoader to avoid
                    // adding all application dirs to KIconLoader::global(), to
                    // avoid potential icon name clashes between application
                    // icons
                    //FIXME: leak
                    KIconLoader *customIconLoader = new KIconLoader(appName, QStringList(), this);
                    customIconLoader->addAppDir(appName, path);
                    return QVariant(QIcon(new KIconEngine(variant.toString(), customIconLoader)));
                } else {
                    qWarning() << "Wrong IconThemePath" << path << ": too short or does not end with 'icons'";
                }
            }

            //return just the string hoping that IconItem will know how to interpret it anyways as either a normal icon or a SVG from the theme
            return variant;
        }
    }

    // Most importantly QIcons. Nothing to do for those.
    return variant;
}

void SystemTray::showPlasmoidMenu(QQuickItem *appletInterface)
{
    if (!appletInterface) {
        return;
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet*>();

    QPointF pos = appletInterface->mapToScene(QPointF(15,15));

    if (appletInterface->window() && appletInterface->window()->screen()) {
        pos = appletInterface->window()->mapToGlobal(pos.toPoint());
    } else {
        pos = QPoint();
    }

    QMenu *desktopMenu = new QMenu;
    connect(this, &QObject::destroyed, desktopMenu, &QMenu::close);
    desktopMenu->setAttribute(Qt::WA_DeleteOnClose);

    foreach (QAction *action, applet->contextualActions()) {
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


    //FIXME: systraycontainer?
    Plasma::Applet *systrayApplet = this;

    if (systrayApplet) {
        QMenu *systrayMenu = new QMenu(i18n("System Tray Options"), desktopMenu);

        foreach (QAction *action, systrayApplet->contextualActions()) {
            if (action) {
                systrayMenu->addAction(action);
            }
        }
        if (systrayApplet->actions()->action(QStringLiteral("configure"))) {
            systrayMenu->addAction(systrayApplet->actions()->action(QStringLiteral("configure")));
        }
        if (systrayApplet->actions()->action(QStringLiteral("remove"))) {
            systrayMenu->addAction(systrayApplet->actions()->action(QStringLiteral("remove")));
        }
        desktopMenu->addMenu(systrayMenu);

        if (systrayApplet->containment() && applet->status() >= Plasma::Types::ActiveStatus) {
            QMenu *containmentMenu = new QMenu(i18nc("%1 is the name of the containment", "%1 Options", systrayApplet->containment()->title()), desktopMenu);

            foreach (QAction *action, systrayApplet->containment()->contextualActions()) {
                if (action) {
                    containmentMenu->addAction(action);
                }
            }
            desktopMenu->addMenu(containmentMenu);
        }
    }

    desktopMenu->adjustSize();

    if (QScreen *screen = appletInterface->window()->screen()) {
        const QRect geo = screen->availableGeometry();

        pos = QPoint(qBound(geo.left(), (int)pos.x(), geo.right() - desktopMenu->width()),
                        qBound(geo.top(), (int)pos.y(), geo.bottom() - desktopMenu->height()));
    }

    desktopMenu->popup(pos.toPoint());
}

Q_INVOKABLE QString SystemTray::plasmoidCategory(QQuickItem *appletInterface) const
{
    if (!appletInterface) {
        return "UnknownCategory";
    }

    Plasma::Applet *applet = appletInterface->property("_plasma_applet").value<Plasma::Applet*>();
    if (!applet || !applet->pluginInfo().isValid()) {
        return "UnknownCategory";
    }

    const QString cat = applet->pluginInfo().property(QStringLiteral("X-Plasma-NotificationAreaCategory")).toString();

    if (cat.isEmpty()) {
        return "UnknownCategory";
    }
    return cat;
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
        int x = sjob->parameters()[QStringLiteral("x")].toInt();
        int y = sjob->parameters()[QStringLiteral("y")].toInt();

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

void SystemTray::restoreContents(KConfigGroup &group)
{
    //Don't do anything here, it's too soon
    qWarning()<<"RestoreContents doesn't do anything here";
}

void SystemTray::restorePlasmoids()
{
    if (!isContainment()) {
        qWarning() << "Loaded as an applet, this shouldn't have happened";
        return;
    }

    //First: remove all that are not allowed anymore
    QStringList tasksToDelete;
    foreach (Plasma::Applet *applet, applets()) {
        //Here it should always be valid.
        //for some reason it not always is.
        if (!applet->pluginInfo().isValid()) {
            applet->destroy();
        } else {
            const QString task = applet->pluginInfo().pluginName();
            if (!m_allowedPlasmoids.contains(task)) {
                applet->destroy();
            }
        }
    }

    KConfigGroup cg = config();

    cg = KConfigGroup(&cg, "Applets");

    foreach (const QString &group, cg.groupList()) {
        KConfigGroup appletConfig(&cg, group);
        QString plugin = appletConfig.readEntry("plugin");
        if (!plugin.isEmpty()) {
            m_knownPlugins[plugin] = group.toInt();
        }
    }
    qWarning() << "Known plasmoid ids:"<< m_knownPlugins;

    //X-Plasma-NotificationArea
    const QString constraint = QStringLiteral("[X-Plasma-NotificationArea] == true");

    KPluginInfo::List applets;
    for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
        if (info.isValid() && info.property(QStringLiteral("X-Plasma-NotificationArea")).toBool() == true) {
            applets << info;
        }
    }

    QStringList ownApplets;

    QMap<QString, KPluginInfo> sortedApplets;
    foreach (const KPluginInfo &info, applets) {
        const QString dbusactivation = info.property(QStringLiteral("X-Plasma-DBusActivationService")).toString();
        if (!dbusactivation.isEmpty()) {
            qCDebug(SYSTEMTRAY) << "ST Found DBus-able Applet: " << info.pluginName() << dbusactivation;
            m_dbusActivatableTasks[info.pluginName()] = dbusactivation;
            continue;
        }

        if (m_allowedPlasmoids.contains(info.pluginName()) &&
            //FIXME
            //!m_tasks.contains(info.pluginName()) &&
            dbusactivation.isEmpty()) {
            // if we already have a plugin with this exact name in it, then check if it is the
            // same plugin and skip it if it is indeed already listed
            if (sortedApplets.contains(info.name())) {

                bool dupe = false;
                // it is possible (though poor form) to have multiple applets
                // with the same visible name but different plugins, so we hve to check all values
                foreach (const KPluginInfo &existingInfo, sortedApplets.values(info.name())) {
                    if (existingInfo.pluginName() == info.pluginName()) {
                        dupe = true;
                        break;
                    }
                }

                if (dupe) {
                    continue;
                }
            }

            // insertMulti becase it is possible (though poor form) to have multiple applets
            // with the same visible name but different plugins
            sortedApplets.insertMulti(info.name(), info);
        }
    }

    foreach (const KPluginInfo &info, sortedApplets) {
        qCDebug(SYSTEMTRAY) << " Adding applet: " << info.name();
        qCDebug(SYSTEMTRAY) << "\n\n ==========================================================================================";
        if (m_allowedPlasmoids.contains(info.pluginName())) {
            newTask(info.pluginName());
        }
    }

    initDBusActivatables();
}

QStringList SystemTray::defaultPlasmoids() const
{
    QStringList ret;
    for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
        if (info.isValid() && info.property(QStringLiteral("X-Plasma-NotificationArea")) == "true" &&
            info.isPluginEnabledByDefault()) {
            ret += info.pluginName();
        }
    }

    return ret;
}

QAbstractItemModel* SystemTray::availablePlasmoids()
{
    if (!m_availablePlasmoidsModel) {
        m_availablePlasmoidsModel = new PlasmoidModel(this);

        //Filter X-Plasma-NotificationArea
        KPluginInfo::List applets;
        for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
            if (info.property(QStringLiteral("X-Plasma-NotificationArea")) == "true") {
                applets << info;
            }
        }

        foreach (const KPluginInfo &info, applets) {
            QString name = info.name();
            KService::Ptr service = info.service();
            const QString dbusactivation = info.property(QStringLiteral("X-Plasma-DBusActivationService")).toString();

            if (!dbusactivation.isEmpty()) {
                name += i18n(" (Automatic load)");
            }
            QStandardItem *item = new QStandardItem(QIcon::fromTheme(info.icon()), name);
            item->setData(info.pluginName());
            m_availablePlasmoidsModel->appendRow(item);
        }
    }
    return m_availablePlasmoidsModel;
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

void SystemTray::initDBusActivatables()
{
    /* Loading and unloading Plasmoids when dbus services come and go
     *
     * This works as follows:
     * - we collect a list of plugins and related services in m_dbusActivatableTasks
     * - we query DBus for the list of services, async (initDBusActivatables())
     * - we go over that list, adding tasks when a service and plugin match (serviceNameFetchFinished())
     * - we start watching for new services, and do the same (serviceNameFetchFinished())
     * - whenever a service is gone, we check whether to unload a Plasmoid (serviceUnregistered())
     */
    QDBusPendingCall async = QDBusConnection::sessionBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished,
            [=](QDBusPendingCallWatcher *callWatcher){
                SystemTray::serviceNameFetchFinished(callWatcher, QDBusConnection::sessionBus());
            });

    QDBusPendingCall systemAsync = QDBusConnection::systemBus().interface()->asyncCall(QStringLiteral("ListNames"));
    QDBusPendingCallWatcher *systemCallWatcher = new QDBusPendingCallWatcher(systemAsync, this);
    connect(systemCallWatcher, &QDBusPendingCallWatcher::finished,
            [=](QDBusPendingCallWatcher *callWatcher){
                SystemTray::serviceNameFetchFinished(callWatcher, QDBusConnection::systemBus());
            });
}

void SystemTray::serviceNameFetchFinished(QDBusPendingCallWatcher* watcher, const QDBusConnection &connection)
{
    QDBusPendingReply<QStringList> propsReply = *watcher;
    watcher->deleteLater();

    if (propsReply.isError()) {
        qCWarning(SYSTEMTRAY) << "Could not get list of available D-Bus services";
    } else {
        foreach (const QString& serviceName, propsReply.value()) {
            serviceRegistered(serviceName);
        }
    }

    // Watch for new services
    // We need to watch for all of new services here, since we want to "match" the names,
    // not just compare them
    // This makes mpris work, since it wants to match org.mpris.MediaPlayer2.dragonplayer
    // against org.mpris.MediaPlayer2
    // QDBusServiceWatcher is not capable for watching wildcard service right now
    // See:
    // https://bugreports.qt.io/browse/QTBUG-51683
    // https://bugreports.qt.io/browse/QTBUG-33829
    connect(connection.interface(), &QDBusConnectionInterface::serviceOwnerChanged, this, &SystemTray::serviceOwnerChanged);
}

void SystemTray::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    if (oldOwner.isEmpty()) {
        serviceRegistered(serviceName);
    } else if (newOwner.isEmpty()) {
        serviceUnregistered(serviceName);
    }
}

void SystemTray::serviceRegistered(const QString &service)
{
    qDebug() << "DBus service appeared:" << service;
    foreach (const QString &plugin, m_dbusActivatableTasks.keys()) {
        if (!m_allowedPlasmoids.contains(plugin)) {
            continue;
        }
        const QString& pattern = m_dbusActivatableTasks.value(plugin);
        QRegExp rx(pattern);
        rx.setPatternSyntax(QRegExp::Wildcard);
        if (rx.exactMatch(service)) {
            qDebug() << "ST : DBus service " << m_dbusActivatableTasks[plugin] << "appeared. Loading " << plugin;
            newTask(plugin);
            m_dbusServiceCounts[plugin]++;
        }
    }
}

void SystemTray::serviceUnregistered(const QString &service)
{
    qDebug() << "DBus service disappeared:" << service;
    foreach (const QString &plugin, m_dbusActivatableTasks.keys()) {
        if (!m_allowedPlasmoids.contains(plugin)) {
            continue;
        }
        const QString& pattern = m_dbusActivatableTasks.value(plugin);
        QRegExp rx(pattern);
        rx.setPatternSyntax(QRegExp::Wildcard);
        if (rx.exactMatch(service)) {
            m_dbusServiceCounts[plugin]--;
            Q_ASSERT(m_dbusServiceCounts[plugin] >= 0);
            if (m_dbusServiceCounts[plugin] == 0) {
                qDebug() << "ST : DBus service " << m_dbusActivatableTasks[plugin] << " disappeared. Unloading " << plugin;
                cleanupTask(plugin);
            }
        }
    }
}

K_EXPORT_PLASMA_APPLET_WITH_JSON(systemtray, SystemTray, "metadata.json")

#include "systemtray.moc"
