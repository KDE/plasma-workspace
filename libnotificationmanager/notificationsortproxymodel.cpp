/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "notificationsortproxymodel_p.h"

#include <QDateTime>

#include "notifications.h"

using namespace NotificationManager;

NotificationSortProxyModel::NotificationSortProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(true);
    sort(0);
}

NotificationSortProxyModel::~NotificationSortProxyModel() = default;

Notifications::SortMode NotificationSortProxyModel::sortMode() const
{
    return m_sortMode;
}

void NotificationSortProxyModel::setSortMode(Notifications::SortMode sortMode)
{
    if (m_sortMode != sortMode) {
        m_sortMode = sortMode;
        invalidate();
        Q_EMIT sortModeChanged();
    }
}

Qt::SortOrder NotificationSortProxyModel::sortOrder() const
{
    return m_sortOrder;
}

void NotificationSortProxyModel::setSortOrder(Qt::SortOrder sortOrder)
{
    if (m_sortOrder != sortOrder) {
        m_sortOrder = sortOrder;
        invalidate();
        Q_EMIT sortOrderChanged();
    }
}

int sortScore(const QModelIndex &idx)
{
    const auto urgency = idx.data(Notifications::UrgencyRole).toInt();
    if (urgency == Notifications::CriticalUrgency) {
        return 3;
    }

    if (idx.data(Notifications::TypeRole).toInt() == Notifications::JobType) {
        return 2;
    }

    if (urgency == Notifications::NormalUrgency) {
        return 1;
    }

    if (urgency == Notifications::LowUrgency) {
        return 0;
    }

    return -1;
}

bool NotificationSortProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    // Sort order is (descending):
    // - Critical notifications
    // - Jobs
    // - Normal notifications
    // - Low urgency notifications
    // Within each group it's descending by created or last modified

    int scoreLeft = 0;
    int scoreRight = 0;

    if (m_sortMode == Notifications::SortByTypeAndUrgency) {
        scoreLeft = sortScore(source_left);
        Q_ASSERT(scoreLeft >= 0);
        scoreRight = sortScore(source_right);
        Q_ASSERT(scoreRight >= 0);
    }

    if (scoreLeft == scoreRight) {
        const QDateTime timeLeft = source_left.data(Notifications::CreatedRole).toDateTime();
        const QDateTime timeRight = source_right.data(Notifications::CreatedRole).toDateTime();

        if (m_sortOrder == Qt::DescendingOrder) {
            return timeLeft > timeRight;
        } else {
            return timeLeft < timeRight;
        }
    }

    return scoreLeft > scoreRight;
}
