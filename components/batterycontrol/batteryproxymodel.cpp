/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "batteryproxymodel.h"
#include "batterycontrol.h"

BatteryProxyModel::BatteryProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortLocaleAware(true);
    setSortRole(BatteryControlModel::PrettyName);
    sort(0);
}

BatteryProxyModel::~BatteryProxyModel() = default;

bool BatteryProxyModel::powerSupplyFirst() const
{
    return m_powerSupplyFirst;
}

void BatteryProxyModel::setPowerSupplyFirst(bool powerSupplyFirst)
{
    if (m_powerSupplyFirst != powerSupplyFirst) {
        m_powerSupplyFirst = powerSupplyFirst;
        invalidate();
        Q_EMIT powerSupplyFirstChanged(powerSupplyFirst);
    }
}

bool BatteryProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const
{
    if (m_powerSupplyFirst) {
        const bool leftPowerSupply = sourceLeft.data(BatteryControlModel::IsPowerSupply).toBool();
        const bool rightPowerSupply = sourceRight.data(BatteryControlModel::IsPowerSupply).toBool();
        if (leftPowerSupply != rightPowerSupply) {
            return leftPowerSupply;
        }
    }

    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

#include "moc_batteryproxymodel.cpp"
