/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "usermodel.h"

#include <KLocalizedString>
#include <QDBusPendingReply>
#include <algorithm>

#include "accounts_interface.h"
#include "kcmusers_debug.h"

UserModel::UserModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new OrgFreedesktopAccountsInterface(QStringLiteral("org.freedesktop.Accounts"),
                                                          QStringLiteral("/org/freedesktop/Accounts"),
                                                          QDBusConnection::systemBus(),
                                                          this))
{
    connect(m_dbusInterface, &OrgFreedesktopAccountsInterface::UserAdded, this, [this](const QDBusObjectPath &path) {
        User *user = new User(this);
        user->setPath(path);
        beginInsertRows(QModelIndex(), m_userList.size(), m_userList.size());
        m_userList.append(user);
        endInsertRows();
    });

    connect(m_dbusInterface, &OrgFreedesktopAccountsInterface::UserDeleted, this, [this](const QDBusObjectPath &path) {
        QList<User *> toRemove;
        for (auto user : std::as_const(m_userList)) {
            if (user->path().path() == path.path()) {
                toRemove << user;
            }
        }
        for (auto user : toRemove) {
            auto index = m_userList.indexOf(user);
            beginRemoveRows(QModelIndex(), index, index);
            m_userList.removeAt(index);
            endRemoveRows();
        }
    });

    auto reply = m_dbusInterface->ListCachedUsers();
    reply.waitForFinished();

    if (reply.isError()) {
        qCWarning(KCMUSERS) << reply.error().message();
        return;
    }

    const QList<QDBusObjectPath> users = reply.value();
    for (const QDBusObjectPath &path : users) {
        User *user = new User(this);
        user->setPath(path);

        struct SignalEnumMapping {
            void (User::*signalFnc)();
            int role;
        };
        static constexpr const std::array<SignalEnumMapping, 8> set{{
            {&User::uidChanged, UidRole},
            {&User::nameChanged, NameRole},
            {&User::displayNamesChanged, DisplayPrimaryNameRole},
            {&User::displayNamesChanged, DisplaySecondaryNameRole},
            {&User::faceValidChanged, FaceValidRole},
            {&User::realNameChanged, RealNameRole},
            {&User::emailChanged, EmailRole},
            {&User::administratorChanged, AdministratorRole},
        }};

        for (const SignalEnumMapping &entry : set) {
            connect(user, entry.signalFnc, this, [this, user, entry] {
                auto idx = index(m_userList.lastIndexOf(user));
                Q_EMIT dataChanged(idx, idx, {entry.role});
            });
        }

        m_userList.append(user);
    }

    std::ranges::stable_partition(m_userList, [](User *u) {
        return u->isCurrentUser();
    });

    connect(this, &QAbstractItemModel::rowsInserted, this, &UserModel::moreThanOneAdminUserChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &UserModel::moreThanOneAdminUserChanged);
    connect(this, &QAbstractItemModel::dataChanged, this, &UserModel::moreThanOneAdminUserChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &UserModel::moreThanOneAdminUserChanged);
}

QHash<int, QByteArray> UserModel::roleNames() const
{
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(UidRole, QByteArrayLiteral("uid"));
    names.insert(NameRole, QByteArrayLiteral("name"));
    names.insert(DisplayPrimaryNameRole, QByteArrayLiteral("displayPrimaryName"));
    names.insert(DisplaySecondaryNameRole, QByteArrayLiteral("displaySecondaryName"));
    names.insert(EmailRole, QByteArrayLiteral("email"));
    names.insert(AdministratorRole, QByteArrayLiteral("administrator"));
    names.insert(UserRole, QByteArrayLiteral("userObject"));
    names.insert(FaceValidRole, QByteArrayLiteral("faceValid"));
    names.insert(IsCurrentUserRole, QByteArrayLiteral("isCurrentUser"));
    names.insert(LoggedInRole, QByteArrayLiteral("loggedIn"));
    names.insert(SectionHeaderRole, QByteArrayLiteral("sectionHeader"));
    return names;
}

UserModel::~UserModel() = default;

User *UserModel::getCurrentUser() const
{
    for (const auto user : std::as_const(m_userList)) {
        if (user->isCurrentUser()) {
            return user;
        }
    }
    return nullptr;
}

bool UserModel::hasMoreThanOneAdminUser() const
{
    int c = 0;
    for (const auto *user : std::as_const(m_userList)) {
        if (user->administrator()) {
            c++;
        }
        if (c > 1) {
            return true;
        }
    }
    return false;
}

QVariant UserModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index)) {
        return {};
    }

    User *user = m_userList.at(index.row());

    switch (role) {
    case NameRole:
        return user->name();
    case FaceRole:
        return user->face().toString();
    case RealNameRole:
        return user->realName();
    case DisplayPrimaryNameRole:
        return user->displayPrimaryName();
    case DisplaySecondaryNameRole:
        return user->displaySecondaryName();
    case EmailRole:
        return user->email();
    case AdministratorRole:
        return user->administrator();
    case FaceValidRole:
        return QFile::exists(user->face().path());
    case UserRole:
        return QVariant::fromValue(user);
    case IsCurrentUserRole:
        return user->isCurrentUser();
    case LoggedInRole:
        return user->loggedIn();
    case SectionHeaderRole:
        return user->isCurrentUser() ? i18n("Your Account") : i18n("Other Accounts");
    }

    return {};
}

int UserModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_userList.count();
}

#include "moc_usermodel.cpp"
