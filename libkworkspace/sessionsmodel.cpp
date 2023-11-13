/*
 SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sessionsmodel.h"
#include "login1_manager_interface.h"
#include "sessionmanagementbackend.h"

#include <KUser>
#include <QDBusConnection>
#include <QDBusObjectPath>

class LogindSessionsModel;

class SessionsModelPrivate
{
public:
    bool includeUnusedSessions = false;
    LogindSessionsModel *logindModel = nullptr;
};

class OrgFreedesktopLogin1ManagerInterface;

struct SessEnt {
    QDBusObjectPath sessionPath;
    QString display;
    QString user;
    QString session;
    int vt = -1;
    bool self : 1 = false;
    bool tty : 1 = false;
};
typedef QList<SessEnt> SessList;

class LogindSessionsModel : public QAbstractListModel
{
    Q_OBJECT
public:
    LogindSessionsModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

    void activate(int row);
    void startNewSession();

private:
    void addSession(const QDBusObjectPath &sessionPath);
    void removeSession(const QDBusObjectPath &sessionPath);

    QList<SessEnt> m_data;
    OrgFreedesktopLogin1ManagerInterface *m_managerIface;
};

SessionsModel::SessionsModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new SessionsModelPrivate)
{
    if (LogindSessionBackend::exists()) {
        d->logindModel = new LogindSessionsModel(this);
        setSourceModel(d->logindModel);
    }
    connect(this, &QSortFilterProxyModel::rowsInserted, this, &SessionsModel::countChanged);
    connect(this, &QSortFilterProxyModel::rowsRemoved, this, &SessionsModel::countChanged);
    connect(this, &QSortFilterProxyModel::modelReset, this, &SessionsModel::countChanged);
}

int SessionsModel::count() const
{
    return rowCount();
}

bool SessionsModel::includeUnusedSessions() const
{
    return d->includeUnusedSessions;
}

void SessionsModel::setIncludeUnusedSessions(bool includeUnusedSessions)
{
    if (d->includeUnusedSessions == includeUnusedSessions) {
        return;
    }
    d->includeUnusedSessions = includeUnusedSessions;
    Q_EMIT includeUnusedSessionsChanged();
    invalidateFilter();
}

void SessionsModel::activate(int row)
{
    if (!d->logindModel) {
        return;
    }
    d->logindModel->activate(row);
}

void SessionsModel::startNewSession()
{
    if (!d->logindModel) {
        return;
    }
    d->logindModel->startNewSession();
}

bool SessionsModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    // this name comes from existing code, I don't understand the name
    if (!d->includeUnusedSessions) {
        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }
    // we should remove self apparently (so why load it?)

    const QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

    int vt = sourceModel()->data(index, SessionsModel::Roles::VtNumberRole).toInt();
    return vt >= 0;
}

LogindSessionsModel::LogindSessionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qDBusRegisterMetaType<SessionInfo>();
    qDBusRegisterMetaType<SessionInfoList>();

    m_managerIface = new OrgFreedesktopLogin1ManagerInterface(QString(), QString(), QDBusConnection::systemBus(), this);

    // existing users rely on blocking implementations, check users making the obvious port
    auto pendingSessions = m_managerIface->ListSessions();
    pendingSessions.waitForFinished();
    for (const SessionInfo &session : pendingSessions.value()) {
        addSession(session.sessionPath);
    }

    connect(m_managerIface, &OrgFreedesktopLogin1ManagerInterface::SessionNew, this, [this](const QString &, const QDBusObjectPath &path) {
        addSession(path);
    });
    connect(m_managerIface, &OrgFreedesktopLogin1ManagerInterface::SessionRemoved, this, [this](const QString &, const QDBusObjectPath &path) {
        removeSession(path);
    });
}

int LogindSessionsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_data.count();
}

QVariant LogindSessionsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::ParentIsInvalid)) {
        return QVariant();
    }
    switch (role) {
    case SessionsModel::Roles::NameRole:
        return m_data[index.row()].user;
    case SessionsModel::Roles::RealNameRole:
        return KUser(m_data[index.row()].user).property(KUser::FullName);
    case SessionsModel::Roles::VtNumberRole:
        return m_data[index.row()].vt;
    case SessionsModel::Roles::SessionRole:
        return m_data[index.row()].session;
    case SessionsModel::Roles::IsTtyRole:
        return m_data[index.row()].tty;
    case SessionsModel::Roles::IconNameRole:
        return QStringLiteral("system-switch-user");
    }
    return QVariant();
}

QHash<int, QByteArray> LogindSessionsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;

    roleNames[SessionsModel::Roles::NameRole] = QByteArrayLiteral("name");
    roleNames[SessionsModel::Roles::RealNameRole] = QByteArrayLiteral("realName");
    roleNames[SessionsModel::Roles::IconRole] = QByteArrayLiteral("icon");
    roleNames[SessionsModel::Roles::IconNameRole] = QByteArrayLiteral("iconName");
    roleNames[SessionsModel::Roles::VtNumberRole] = QByteArrayLiteral("vtNumber");
    roleNames[SessionsModel::Roles::SessionRole] = QByteArrayLiteral("session");
    roleNames[SessionsModel::Roles::IsTtyRole] = QByteArrayLiteral("isTty");

    return roleNames;
}

void LogindSessionsModel::activate(int row)
{
    if (row < 0 || row >= m_data.count()) {
        qCritical() << "Invalid call to activate session " << row;
        return;
    }
    QDBusMessage message =
        QDBusMessage::createMethodCall(m_managerIface->service(), m_data[row].sessionPath.path(), LogindSessionBackend::sessionIfaceName(), "Activate");

    // old code had logic to lock before switching, is it needed?
    QDBusConnection::systemBus().asyncCall(message);
}

void LogindSessionsModel::startNewSession()
{
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DisplayManager"),
                                                          qgetenv("XDG_SEAT_PATH"),
                                                          QStringLiteral("org.freedesktop.DisplayManager.Seat"),
                                                          "SwitchToGreeter");

    QDBusConnection::systemBus().asyncCall(message);
}

void LogindSessionsModel::addSession(const QDBusObjectPath &sessionPath)
{
    QDBusMessage message = QDBusMessage::createMethodCall(m_managerIface->service(), sessionPath.path(), "org.freedesktop.DBus.Properties", "GetAll");

    message << LogindSessionBackend::sessionIfaceName();

    // existing code is blocking, therefore this is for now
    // fixing this means changing the krunner engine

    QDBusPendingReply<QVariantMap> reply = QDBusConnection::systemBus().asyncCall(message);
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Error getting session properties" << reply.error();
        return;
    }
    QVariantMap properties = reply.value();

    if (properties["Id"].toString() == qgetenv("XDG_SESSION_ID")) {
        // ignore self
        return;
    }

    SessEnt se;
    se.sessionPath = sessionPath;
    se.display = properties["Display"].toString();
    se.vt = properties["VTNr"].toInt();
    se.user = properties["Name"].toString();
    se.tty = properties["TTY"].toBool();

    beginInsertRows(QModelIndex(), m_data.count(), m_data.count());
    m_data << se;
    endInsertRows();
}

void LogindSessionsModel::removeSession(const QDBusObjectPath &sessionPath)
{
    for (int i = 0; i < m_data.count(); i++) {
        if (m_data[i].sessionPath == sessionPath) {
            beginRemoveRows(QModelIndex(), i, i);
            m_data.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}

#include "sessionsmodel.moc"
