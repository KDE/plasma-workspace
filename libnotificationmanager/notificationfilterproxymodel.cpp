/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
        Q_EMIT urgenciesChanged();
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
        Q_EMIT showExpiredChanged();
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
        Q_EMIT showDismissedChanged();
    }
}

bool NotificationFilterProxyModel::showAddedDuringInhibition() const
{
    return m_showDismissed;
}

void NotificationFilterProxyModel::setShowAddedDuringInhibition(bool show)
{
    if (m_showAddedDuringInhibition != show) {
        m_showAddedDuringInhibition = show;
        invalidateFilter();
        Q_EMIT showAddedDuringInhibitionChanged();
    }
}

bool NotificationFilterProxyModel::ignoreBlacklistDuringInhibition() const
{
    return m_ignoreBlacklistDuringInhibition;
}

void NotificationFilterProxyModel::setIgnoreBlacklistDuringInhibition(bool ignore)
{
    if (m_ignoreBlacklistDuringInhibition != ignore) {
        m_ignoreBlacklistDuringInhibition = ignore;
        invalidateFilter();
        Q_EMIT ignoreBlacklistDuringInhibitionChanged();
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
        Q_EMIT blacklistedDesktopEntriesChanged();
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
        Q_EMIT blacklistedNotifyRcNamesChanged();
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
        Q_EMIT whitelistedDesktopEntriesChanged();
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
        Q_EMIT whitelistedNotifyRcNamesChanged();
    }
}

bool NotificationFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!sourceModel()->hasIndex(source_row, 0, source_parent)) {
        return false;
    }
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

    bool ignoreBlacklist = m_ignoreBlacklistDuringInhibition && sourceIdx.data(Notifications::WasAddedDuringInhibitionRole).toBool();

    // Blacklist takes precedence over whitelist, i.e. when in doubt don't show
    if (!m_blacklistedDesktopEntries.isEmpty() && !ignoreBlacklist) {
        if (!desktopEntry.isEmpty() && m_blacklistedDesktopEntries.contains(desktopEntry)) {
            return false;
        }
    }

    if (!m_blacklistedNotifyRcNames.isEmpty() && !ignoreBlacklist) {
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
        if (!m_urgencies.testFlag(urgency) && !ignoreBlacklist) {
            return false;
        }
    }

    // Normal Do Not Disturb filtering
    if (!m_showAddedDuringInhibition) {
        // Show critical notifications even in Do Not Disturb
        if (!m_urgencies.testFlag(urgency) && sourceIdx.data(Notifications::WasAddedDuringInhibitionRole).toBool()) {
            return false;
        }
    }

    return true;
}

#include "moc_notificationfilterproxymodel_p.cpp"
