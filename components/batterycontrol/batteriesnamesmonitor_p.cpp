/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "batteriesnamesmonitor_p.h"

#include <Solid/Device>
#include <klocalizedstring.h>

QString BatteriesNamesMonitor::updateBatteryName(const Solid::Device &deviceBattery, Solid::Battery *battery)
{
    // Don't show battery name for primary power supply batteries. They usually have cryptic serial number names.
    bool showBatteryName = battery->type() != Solid::Battery::PrimaryBattery || !battery->isPowerSupply();
    if (!deviceBattery.product().isEmpty() && deviceBattery.product() != QLatin1String("Unknown Battery") && showBatteryName) {
        if (!deviceBattery.vendor().isEmpty()) {
            return (QString(deviceBattery.vendor() + ' ' + deviceBattery.product()));
        }
        return deviceBattery.product();
    }
    ++m_unnamedBatteriesCount;
    m_unnamedBatteries[deviceBattery.udi()] = m_unnamedBatteriesCount;
    if (m_unnamedBatteriesCount > 1) {
        return i18nc("Placeholder is the battery number", "Battery %1", m_freeNames.isEmpty() ? m_unnamedBatteriesCount : m_freeNames.dequeue());
    }
    return i18n("Battery");
}

void BatteriesNamesMonitor::removeBatteryName(const QString &udi)
{
    if (m_unnamedBatteriesCount > 1) {
        auto position = m_unnamedBatteries.constFind(udi);
        if (position != m_unnamedBatteries.constEnd()) {
            if (*position == m_unnamedBatteriesCount) {
                --m_unnamedBatteriesCount;
            } else {
                if (*position != 1) {
                    m_freeNames.enqueue(*position);
                }
            }
            m_unnamedBatteries.erase(position);
        }
    }
}
