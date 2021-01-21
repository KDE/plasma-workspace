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
namespace Plasma
{
class Service;
}
class PlasmoidRegistry;
class PlasmoidModel;
class SystemTraySettings;
class StatusNotifierModel;
class SystemTrayModel;
class SortedSystemTrayModel;

class SystemTray : public Plasma::Containment
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *systemTrayModel READ sortedSystemTrayModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *configSystemTrayModel READ configSystemTrayModel CONSTANT)

public:
    SystemTray(QObject *parent, const QVariantList &args);
    ~SystemTray() override;

    void init() override;

    void restoreContents(KConfigGroup &group) override;

    QAbstractItemModel *sortedSystemTrayModel();

    QAbstractItemModel *configSystemTrayModel();

    // Invokable utilities
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
    Q_INVOKABLE QPointF popupPosition(QQuickItem *visualParent, int x, int y);

    /**
     * @brief isSystemTrayApplet checks if applet is allowed in the System Tray
     * @param appletId also known as plugin Id
     * @return true if it is a system tray applet, otherwise false
     */
    Q_INVOKABLE bool isSystemTrayApplet(const QString &appletId);

private Q_SLOTS:
    // synchronizes with configuration and deletes not allowed applets
    void onEnabledAppletsChanged();
    // creates an applet *if not already existing*
    void startApplet(const QString &pluginId);
    // deletes/stops all instances of a given applet
    void stopApplet(const QString &pluginId);

private:
    SystemTrayModel *systemTrayModel();

    QPointer<SystemTraySettings> m_settings;
    QPointer<PlasmoidRegistry> m_plasmoidRegistry;

    PlasmoidModel *m_plasmoidModel;
    StatusNotifierModel *m_statusNotifierModel;
    SystemTrayModel *m_systemTrayModel;
    SortedSystemTrayModel *m_sortedSystemTrayModel;
    SortedSystemTrayModel *m_configSystemTrayModel;

    QHash<QString /*plugin id*/, int /*config group*/> m_configGroupIds;
};

#endif
