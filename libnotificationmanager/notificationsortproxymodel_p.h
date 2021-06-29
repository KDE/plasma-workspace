/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QSortFilterProxyModel>

#include "notifications.h"

namespace NotificationManager
{
class NotificationSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit NotificationSortProxyModel(QObject *parent = nullptr);
    ~NotificationSortProxyModel() override;

    Notifications::SortMode sortMode() const;
    void setSortMode(Notifications::SortMode);

    Qt::SortOrder sortOrder() const;
    void setSortOrder(Qt::SortOrder sortOrder);

Q_SIGNALS:
    void sortModeChanged();
    void sortOrderChanged();

protected:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

private:
    Notifications::SortMode m_sortMode = Notifications::SortByDate;
    Qt::SortOrder m_sortOrder = Qt::DescendingOrder;
};

} // namespace NotificationManager
