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

signals:
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
    Notifications::Urgencies m_urgencies = Notifications::LowUrgency
            | Notifications::NormalUrgency
            | Notifications::CriticalUrgency;
    bool m_showDismissed = false;
    bool m_showExpired = false;

    QStringList m_blacklistedDesktopEntries;
    QStringList m_blacklistedNotifyRcNames;

    QStringList m_whitelistedDesktopEntries;
    QStringList m_whitelistedNotifyRcNames;

};

} // namespace NotificationManager
