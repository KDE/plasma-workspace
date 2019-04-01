/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "notificationsortproxymodel_p.h"

#include <QDateTime>

#include "notificationmodel.h"
#include "notifications.h"

#include <QDebug>

using namespace NotificationManager;

NotificationSortProxyModel::NotificationSortProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(true);
    sort(0);
    // TODO set roles relevant for sorting
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
        emit sortModeChanged();
    }
}

int sortScore(const QModelIndex &idx)
{
    const auto urgency = idx.data(Notifications::UrgencyRole).toInt();
    if (urgency == Notifications::CriticalUrgency) {
        return 3;
    }

    const int type = idx.data(Notifications::TypeRole).toInt();
    if (type == Notifications::JobType) {
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
        QDateTime timeLeft = source_left.data(Notifications::UpdatedRole).toDateTime();
        if (!timeLeft.isValid()) {
            timeLeft = source_left.data(Notifications::CreatedRole).toDateTime();
        }

        QDateTime timeRight = source_right.data(Notifications::UpdatedRole).toDateTime();
        if (!timeRight.isValid()) {
            timeRight = source_right.data(Notifications::CreatedRole).toDateTime();
        }

        // sorts descending by time (newest first)
        return timeLeft > timeRight;
    }

    return scoreLeft > scoreRight;
}
