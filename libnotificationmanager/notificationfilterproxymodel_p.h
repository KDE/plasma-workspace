/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QSortFilterProxyModel>
#include <QStringList>

#include "notifications.h"

namespace NotificationManager
{
class NotificationFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit NotificationFilterProxyModel(QObject *parent = nullptr);
    ~NotificationFilterProxyModel() override;

    Notifications::Urgencies urgencies() const;
    void setUrgencies(Notifications::Urgencies urgencies);

    bool showExpired() const;
    void setShowExpired(bool show);

    bool showDismissed() const;
    void setShowDismissed(bool show);

    QStringList blacklistedDesktopEntries() const;
    void setBlackListedDesktopEntries(const QStringList &blacklist);

    QStringList blacklistedNotifyRcNames() const;
    void setBlacklistedNotifyRcNames(const QStringList &blacklist);

    QStringList whitelistedDesktopEntries() const;
    void setWhiteListedDesktopEntries(const QStringList &whitelist);

    QStringList whitelistedNotifyRcNames() const;
    void setWhitelistedNotifyRcNames(const QStringList &whitelist);

Q_SIGNALS:
    void urgenciesChanged();
    void showExpiredChanged();
    void showDismissedChanged();
    void blacklistedDesktopEntriesChanged();
    void blacklistedNotifyRcNamesChanged();
    void whitelistedDesktopEntriesChanged();
    void whitelistedNotifyRcNamesChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    Notifications::Urgencies m_urgencies = Notifications::LowUrgency | Notifications::NormalUrgency | Notifications::CriticalUrgency;
    bool m_showDismissed = false;
    bool m_showExpired = false;

    QStringList m_blacklistedDesktopEntries;
    QStringList m_blacklistedNotifyRcNames;

    QStringList m_whitelistedDesktopEntries;
    QStringList m_whitelistedNotifyRcNames;
};

} // namespace NotificationManager
