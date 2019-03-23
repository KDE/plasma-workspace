/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

#include "notificationfilterproxymodel_p.h"

using namespace NotificationManager;

NotificationFilterProxyModel::NotificationFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(true);
}

NotificationFilterProxyModel::~NotificationFilterProxyModel() = default;

Notifications::Urgencies NotificationFilterProxyModel::urgencies() const
{
    return m_urgencies;
}

void NotificationFilterProxyModel::setUrgencies(Notifications::Urgencies urgencies)
{
    if (m_urgencies != urgencies) {
        m_urgencies = urgencies;
        invalidateFilter();
        emit urgenciesChanged();
    }
}

bool NotificationFilterProxyModel::showExpired() const
{
    return m_showExpired;
}

void NotificationFilterProxyModel::setShowExpired(bool show)
{
    if (m_showExpired != show) {
        m_showExpired = show;
        invalidateFilter();
        emit showExpiredChanged();
    }
}

bool NotificationFilterProxyModel::showDismissed() const
{
    return m_showDismissed;
}

void NotificationFilterProxyModel::setShowDismissed(bool show)
{
    if (m_showDismissed != show) {
        m_showDismissed = show;
        invalidateFilter();
        emit showDismissedChanged();
    }
}

bool NotificationFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex sourceIdx = sourceModel()->index(source_row, 0, source_parent);

    if (!m_showExpired && sourceIdx.data(Notifications::ExpiredRole).toBool()) {
        return false;
    }

    if (!m_showDismissed && sourceIdx.data(Notifications::DismissedRole).toBool()) {
        return false;
    }

    // TODO can we just bitshift them back or match them better with some maths rather than a switch?
    bool ok;
    const int urgency = sourceIdx.data(Notifications::UrgencyRole).toInt(&ok);
    if (ok) {
        switch (static_cast<Notifications::Urgencies>(urgency)) {
        case Notifications::LowUrgency:
            if (!m_urgencies.testFlag(Notifications::LowUrgency)) {
                return false;
            }
            break;
        case Notifications::NormalUrgency:
            if (!m_urgencies.testFlag(Notifications::NormalUrgency)) {
                return false;
            }
            break;
        case Notifications::CriticalUrgency:
            if (!m_urgencies.testFlag(Notifications::CriticalUrgency)) {
                return false;
            }
            break;
        }
    }

    return true;
}
