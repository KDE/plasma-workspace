/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>

#include <Solid/Battery>
#include <Solid/Device>

#include <QQueue>

class BatteriesNamesMonitor
{
public:
    QString updateBatteryName(const Solid::Device &deviceBattery, Solid::Battery *battery);
    void removeBatteryName(const QString &udi);

private:
    QHash<QString, uint> m_unnamedBatteries;
    QQueue<uint> m_freeNames;
    uint m_unnamedBatteriesCount = 0;
};
