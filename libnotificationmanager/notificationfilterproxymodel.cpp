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

QStringList NotificationFilterProxyModel::blacklistedDesktopEntries() const
{
    return m_blacklistedDesktopEntries;
}

void NotificationFilterProxyModel::setBlackListedDesktopEntries(const QStringList &blacklist)
{
    if (m_blacklistedDesktopEntries != blacklist) {
        m_blacklistedDesktopEntries = blacklist;
        invalidateFilter();
        emit blacklistedDesktopEntriesChanged();
    }
}

QStringList NotificationFilterProxyModel::blacklistedNotifyRcNames() const
{
    return m_blacklistedNotifyRcNames;
}

void NotificationFilterProxyModel::setBlacklistedNotifyRcNames(const QStringList &blacklist)
{
    if (m_blacklistedNotifyRcNames != blacklist) {
        m_blacklistedNotifyRcNames = blacklist;
        invalidateFilter();
        emit blacklistedNotifyRcNamesChanged();
    }
}

QStringList NotificationFilterProxyModel::whitelistedDesktopEntries() const
{
    return m_whitelistedDesktopEntries;
}

void NotificationFilterProxyModel::setWhiteListedDesktopEntries(const QStringList &whitelist)
{
    if (m_whitelistedDesktopEntries != whitelist) {
        m_whitelistedDesktopEntries = whitelist;
        invalidateFilter();
        emit whitelistedDesktopEntriesChanged();
    }
}

QStringList NotificationFilterProxyModel::whitelistedNotifyRcNames() const
{
    return m_whitelistedNotifyRcNames;
}

void NotificationFilterProxyModel::setWhitelistedNotifyRcNames(const QStringList &whitelist)
{
    if (m_whitelistedNotifyRcNames != whitelist) {
        m_whitelistedNotifyRcNames = whitelist;
        invalidateFilter();
        emit whitelistedNotifyRcNamesChanged();
    }
}

bool NotificationFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex sourceIdx = sourceModel()->index(source_row, 0, source_parent);

    const bool expired = sourceIdx.data(Notifications::ExpiredRole).toBool();
    if (!m_showExpired && expired) {
        return false;
    }

    if (!m_showDismissed && sourceIdx.data(Notifications::DismissedRole).toBool()) {
        return false;
    }

    QString desktopEntry = sourceIdx.data(Notifications::DesktopEntryRole).toString();
    if (desktopEntry.isEmpty()) {
        // For non-configurable notifications use the fake "@other" category.
        if (!sourceIdx.data(Notifications::ConfigurableRole).toBool()
            // jobs are never configurable so this only applies to notifications
            && sourceIdx.data(Notifications::TypeRole).toInt() == Notifications::NotificationType) {
            desktopEntry = QStringLiteral("@other");
        }
    }

    // Blacklist takes precedence over whitelist, i.e. when in doubt don't show
    if (!m_blacklistedDesktopEntries.isEmpty()) {
        if (!desktopEntry.isEmpty() && m_blacklistedDesktopEntries.contains(desktopEntry)) {
            return false;
        }
    }

    if (!m_blacklistedNotifyRcNames.isEmpty()) {
        const QString notifyRcName = sourceIdx.data(Notifications::NotifyRcNameRole).toString();
        if (!notifyRcName.isEmpty() && m_blacklistedNotifyRcNames.contains(notifyRcName)) {
            return false;
        }
    }

    if (!m_whitelistedDesktopEntries.isEmpty()) {
        if (!desktopEntry.isEmpty() && m_whitelistedDesktopEntries.contains(desktopEntry)) {
            return true;
        }
    }

    if (!m_whitelistedNotifyRcNames.isEmpty()) {
        const QString notifyRcName = sourceIdx.data(Notifications::NotifyRcNameRole).toString();
        if (!notifyRcName.isEmpty() && m_whitelistedNotifyRcNames.contains(notifyRcName)) {
            return true;
        }
    }

    const bool userActionFeedback = sourceIdx.data(Notifications::UserActionFeedbackRole).toBool();
    if (userActionFeedback) {
        return true;
    }

    bool ok;
    const auto urgency = static_cast<Notifications::Urgency>(sourceIdx.data(Notifications::UrgencyRole).toInt(&ok));
    if (ok) {
        if (!m_urgencies.testFlag(urgency)) {
            return false;
        }
    }

    return true;
}
