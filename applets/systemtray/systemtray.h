/***************************************************************************
 *   Copyright (C) 2015 Marco Martin <mart@kde.org>                        *
 *
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

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <QAbstractItemModel>
#include <QRegExp>

#include <Plasma/Containment>

class QDBusPendingCallWatcher;
class QDBusServiceWatcher;
class QQuickItem;
namespace Plasma {
    class Service;
}
class PlasmoidModel;
class StatusNotifierModel;
class SystemTrayModel;
class SortedSystemTrayModel;

class SystemTray : public Plasma::Containment
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* systemTrayModel READ sortedSystemTrayModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* configSystemTrayModel READ configSystemTrayModel CONSTANT)
    Q_PROPERTY(QStringList allowedPlasmoids READ allowedPlasmoids WRITE setAllowedPlasmoids NOTIFY allowedPlasmoidsChanged)

public:
    SystemTray( QObject *parent, const QVariantList &args );
    ~SystemTray() override;

    void init() override;

    void restoreContents(KConfigGroup &group) override;
    void restorePlasmoids();

    void configChanged() override;

    QAbstractItemModel *sortedSystemTrayModel();

    QAbstractItemModel *configSystemTrayModel();

    QStringList allowedPlasmoids() const;
    void setAllowedPlasmoids(const QStringList &allowed);

    //Creates an applet *if not already existing*
    void newTask(const QString &task);

    //cleans all instances of a given applet
    void cleanupTask(const QString &task);

    //Invokable utilities
    /**
     * Given an AppletInterface pointer, shows a proper context menu for it
     */
    Q_INVOKABLE void showPlasmoidMenu(QQuickItem *appletInterface, int x, int y);

    /**
     * Shows the context menu for a statusnotifieritem
     */
    Q_INVOKABLE void showStatusNotifierContextMenu(KJob *job, QQuickItem *statusNotifierIcon);

    /**
     * Find out global coordinates for a popup given local MouseArea
     * coordinates
     */
    Q_INVOKABLE QPointF popupPosition(QQuickItem* visualParent, int x, int y);

    Q_INVOKABLE bool isSystemTrayApplet(const QString &appletId);

    /**
     * @returns a Plasma::Service given a source name
     * @param source source name we want a service of
     */
    Q_INVOKABLE Plasma::Service *serviceForSource(const QString &source);

private Q_SLOTS:
    void serviceNameFetchFinished(QDBusPendingCallWatcher* watcher);

private:
    void serviceRegistered(const QString &service);
    void serviceUnregistered(const QString &service);

Q_SIGNALS:
    void allowedPlasmoidsChanged();
    void configurationChanged(const KConfigGroup &config);

private:
    void initDBusActivatables();
    SystemTrayModel *systemTrayModel();

    QStringList m_defaultPlasmoids;
    QHash<QString /*plugin name*/, KPluginMetaData> m_systrayApplets;
    QHash<QString /*plugin name*/, QRegExp /*DBus Service*/> m_dbusActivatableTasks;

    QStringList m_allowedPlasmoids;
    StatusNotifierModel *m_statusNotifierModel;
    SystemTrayModel *m_systemTrayModel;
    SortedSystemTrayModel *m_sortedSystemTrayModel;
    SortedSystemTrayModel *m_configSystemTrayModel;
    QHash<QString, int> m_knownPlugins;

    QDBusServiceWatcher *m_sessionServiceWatcher;
    QDBusServiceWatcher *m_systemServiceWatcher;
    QHash<QString, int> m_dbusServiceCounts;
    bool m_dbusSessionServiceNamesFetched = false;
    bool m_dbusSystemServiceNamesFetched = false;
};

#endif
