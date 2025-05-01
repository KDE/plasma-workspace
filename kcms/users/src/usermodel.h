/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QAbstractListModel>

#include "user.h"

class OrgFreedesktopAccountsInterface;

class UserModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        RealNameRole = Qt::DisplayRole,
        FaceRole = Qt::DecorationRole,
        UidRole = Qt::UserRole,
        NameRole,
        DisplayPrimaryNameRole,
        DisplaySecondaryNameRole,
        EmailRole,
        FaceValidRole,
        AdministratorRole,
        UserRole,
        IsCurrentUserRole,
        LoggedInRole,
        SectionHeaderRole,
    };
    Q_ENUM(Roles)

    Q_PROPERTY(bool moreThanOneAdminUser READ hasMoreThanOneAdminUser NOTIFY moreThanOneAdminUserChanged FINAL)

    explicit UserModel(QObject *parent = nullptr);
    ~UserModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_INVOKABLE User *getCurrentUser() const;

    QHash<int, QByteArray> roleNames() const override;

Q_SIGNALS:
    void moreThanOneAdminUserChanged();

private:
    OrgFreedesktopAccountsInterface *const m_dbusInterface;
    QList<User *> m_userList;

    bool hasMoreThanOneAdminUser() const;
};
