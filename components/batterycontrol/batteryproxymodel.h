/*
 * SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QSortFilterProxyModel>
#include <qqmlregistration.h>

class BatteryProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool powerSupplyFirst READ powerSupplyFirst WRITE setPowerSupplyFirst NOTIFY powerSupplyFirstChanged)

public:
    explicit BatteryProxyModel(QObject *parent = nullptr);
    ~BatteryProxyModel() override;

    bool powerSupplyFirst() const;
    void setPowerSupplyFirst(bool powerSupplyFirst);
    Q_SIGNAL void powerSupplyFirstChanged(bool powerSupplyFirst);

    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;

private:
    bool m_powerSupplyFirst = false;
};
