/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QMap>
#include <QString>

class UPowerBattery;

class BatteriesNamesMonitor
{
public:
    QString updateBatteryName(const UPowerBattery &battery);
    void removeBatteryName(const QString &udi);

private:
    QMap<QString, uint> m_namedBatteries;
};
