/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "batteriesnamesmonitor_p.h"

#include "upowerbattery_p.h"

#include <klocalizedstring.h>

QString BatteriesNamesMonitor::updateBatteryName(const UPowerBattery &battery)
{
    // Don't show battery name for primary power supply batteries. They usually have cryptic serial number names.
    bool showBatteryName = battery.type() != UPowerBattery::Type::Primary || !battery.isPowerSupply();
    const QString product = battery.product();
    if (!product.isEmpty() && product != QLatin1String("Unknown Battery") && showBatteryName) {
        const QString vendor = battery.vendor();
        if (!vendor.isEmpty()) {
            return QString(vendor + QLatin1Char(' ') + product);
        }
        return product;
    }

    // If we can't show a real name, let's do it ourselves
    uint batteryIndex;
    if (auto it = m_namedBatteries.constFind(battery.udi()); it != m_namedBatteries.constEnd()) {
        // We've already named this battery
        batteryIndex = *it;
    } else {
        // Let's name it - use the lowest unused index
        QList<uint> batteryIndexes = m_namedBatteries.values();
        std::ranges::sort(batteryIndexes);

        batteryIndex = 1;
        for (uint currentIndex : batteryIndexes) {
            if (currentIndex == batteryIndex) {
                ++batteryIndex;
            } else {
                break;
            }
        }

        m_namedBatteries.insert(battery.udi(), batteryIndex);
    }

    if (batteryIndex > 1) {
        return i18nc("Placeholder is the battery number", "Battery %1", batteryIndex);
    } else {
        return i18n("Battery");
    }
}

void BatteriesNamesMonitor::removeBatteryName(const QString &udi)
{
    m_namedBatteries.remove(udi);
}
