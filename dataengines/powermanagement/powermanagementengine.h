/*
 *   Copyright 2007 Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007-2008 Sebastian Kuegler <sebas@kde.org>
 *   Copyright 2008 Dario Freddi <drf54321@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#ifndef POWERMANAGEMENTENGINE_H
#define POWERMANAGEMENTENGINE_H

#include <Plasma/DataEngine>

#include <solid/battery.h>
#include <solid/acadapter.h>

#include <QtDBus/QDBusConnection>
#include <QHash>

class QDBusPendingCallWatcher;

/**
 * This class provides runtime information about the battery and AC status
 * for use in power management Plasma applets.
 */
class PowermanagementEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    PowermanagementEngine( QObject* parent, const QVariantList& args );
    ~PowermanagementEngine();
    QStringList sources() const;
    Plasma::Service* serviceForSource(const QString &source);

protected:
    bool sourceRequestEvent(const QString &name);
    bool updateSourceEvent(const QString &source);
    void init();

private Q_SLOTS:
    void updateBatteryChargeState(int newState, const QString& udi);
    void updateBatteryPlugState(bool newState, const QString& udi);
    void updateBatteryChargePercent(int newValue, const QString& udi);
    void updateBatteryPowerSupplyState(bool newState, const QString& udi);
    void updateAcPlugState(bool newState);
    void updateBatteryNames();

    void deviceRemoved(const QString& udi);
    void deviceAdded(const QString& udi);
    void batteryRemainingTimeChanged(qulonglong time);
    void batteryRemainingTimeReply(QDBusPendingCallWatcher*);
    void screenBrightnessChanged(int brightness);
    void keyboardBrightnessChanged(int brightness);
    void screenBrightnessReply(QDBusPendingCallWatcher *watcher);
    void keyboardBrightnessReply(QDBusPendingCallWatcher *watcher);
    void brightnessControlsAvailableChanged(bool available);
    void keyboardBrightnessControlsAvailableChanged(bool available);

private:
    QString batteryType(const Solid::Battery *battery);
    QStringList basicSourceNames() const;

    QStringList m_sources;

    QHash<QString, QString> m_batterySources;  // <udi, Battery0>

    bool m_brightnessControlsAvailable;
    bool m_keyboardBrightnessControlsAvailable;

};


#endif
