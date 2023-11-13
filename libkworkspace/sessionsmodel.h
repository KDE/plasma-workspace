/*
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kworkspace_export.h"
#include <QSortFilterProxyModel>

class SessionsModelPrivate;

class KWORKSPACE_EXPORT SessionsModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(bool includeUnusedSessions READ includeUnusedSessions WRITE setIncludeUnusedSessions NOTIFY includeUnusedSessionsChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles { // These roles match SDDM::SessionModel
        NameRole = Qt::UserRole + 1,
        RealNameRole,
        IconRole, // path to a file
        IconNameRole, // name of an icon
        VtNumberRole,
        SessionRole,
        IsTtyRole,
    };
    Q_ENUM(Roles)

    SessionsModel(QObject *parent = nullptr);

    int count() const;
    bool includeUnusedSessions() const;
    void setIncludeUnusedSessions(bool includeUnusedSessions);

Q_SIGNALS:
    void countChanged();
    void includeUnusedSessionsChanged();

public Q_SLOTS:
    void activate(int row);
    void startNewSession();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    SessionsModelPrivate *d;
};
