/*
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#include "sessionsmodel.h"

#include <utility>

#include <KAuthorized>
#include <KLocalizedString>
#include <KUser>

#include "kscreensaversettings.h"

#include "screensaver_interface.h"

SessionsModel::SessionsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_screensaverInterface(
          new org::freedesktop::ScreenSaver(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus(), this))
{
    reload();

    // wait for the screen locker to be ready before actually switching
    connect(m_screensaverInterface, &org::freedesktop::ScreenSaver::ActiveChanged, this, [this](bool active) {
        if (active) {
            if (m_pendingVt) {
                m_displayManager.switchVT(m_pendingVt);
                Q_EMIT switchedUser(m_pendingVt);
            } else if (m_pendingReserve) {
                m_displayManager.startReserve();
                Q_EMIT startedNewSession();
            }

            m_pendingVt = 0;
            m_pendingReserve = false;
        }
    });
}

bool SessionsModel::canSwitchUser() const
{
    return const_cast<SessionsModel *>(this)->m_displayManager.isSwitchable() && KAuthorized::authorizeAction(QStringLiteral("switch_user"));
}

bool SessionsModel::canStartNewSession() const
{
    return const_cast<SessionsModel *>(this)->m_displayManager.numReserve() > 0 && KAuthorized::authorizeAction(QStringLiteral("start_new_session"));
}

bool SessionsModel::shouldLock() const
{
    return m_shouldLock;
}

bool SessionsModel::includeUnusedSessions() const
{
    return m_includeUnusedSessions;
}

void SessionsModel::setIncludeUnusedSessions(bool includeUnusedSessions)
{
    if (m_includeUnusedSessions != includeUnusedSessions) {
        m_includeUnusedSessions = includeUnusedSessions;

        reload();

        Q_EMIT includeUnusedSessionsChanged();
    }
}

void SessionsModel::switchUser(int vt, bool shouldLock)
{
    if (vt < 0) {
        startNewSession(shouldLock);
        return;
    }

    if (!canSwitchUser()) {
        return;
    }

    if (!shouldLock) {
        m_displayManager.switchVT(vt);
        Q_EMIT switchedUser(vt);
        return;
    }

    checkScreenLocked([this, vt](bool locked) {
        if (locked) {
            // already locked, switch right away
            m_displayManager.switchVT(vt);
            Q_EMIT switchedUser(vt);
        } else {
            m_pendingReserve = false;
            m_pendingVt = vt;

            Q_EMIT aboutToLockScreen();
            m_screensaverInterface->Lock();
        }
    });
}

void SessionsModel::startNewSession(bool shouldLock)
{
    if (!canStartNewSession()) {
        return;
    }

    if (!shouldLock) {
        m_displayManager.startReserve();
        Q_EMIT startedNewSession();
        return;
    }

    checkScreenLocked([this](bool locked) {
        if (locked) {
            // already locked, switch right away
            m_displayManager.startReserve();
            Q_EMIT startedNewSession();
        } else {
            m_pendingReserve = true;
            m_pendingVt = 0;

            Q_EMIT aboutToLockScreen();
            m_screensaverInterface->Lock();
        }
    });
}

void SessionsModel::reload()
{
    static QHash<QString, KUser> kusers;

    const bool oldShouldLock = m_shouldLock;
    m_shouldLock = KAuthorized::authorizeAction(QStringLiteral("lock_screen")) && KScreenSaverSettings::autolock();
    if (m_shouldLock != oldShouldLock) {
        Q_EMIT shouldLockChanged();
    }

    SessList sessions;
    m_displayManager.localSessions(sessions);

    const int oldCount = m_data.count();

    beginResetModel();

    m_data.clear();
    m_data.reserve(sessions.count());

    for (const SessEnt &session : std::as_const(sessions)) {
        if (!session.vt || session.self) {
            continue;
        }

        if (!m_includeUnusedSessions && session.session.isEmpty()) {
            continue;
        }

        SessionEntry entry;
        entry.name = session.user;
        entry.displayNumber = session.display;
        entry.vtNumber = session.vt;
        entry.session = session.session;
        entry.isTty = session.tty;

        auto it = kusers.constFind(session.user);
        if (it != kusers.constEnd()) {
            entry.realName = it->property(KUser::FullName).toString();
            entry.icon = it->faceIconPath();
        } else {
            KUser user(session.user);
            entry.realName = user.property(KUser::FullName).toString();
            entry.icon = user.faceIconPath();
            kusers.insert(session.user, user);
        }

        m_data.append(entry);
    }

    endResetModel();

    if (oldCount != m_data.count()) {
        Q_EMIT countChanged();
    }
}

void SessionsModel::checkScreenLocked(const std::function<void(bool)> &cb)
{
    auto reply = m_screensaverInterface->GetActive();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [cb](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<bool> reply = *watcher;
        if (!reply.isError()) {
            cb(reply.value());
        }
        watcher->deleteLater();
    });
}

void SessionsModel::setShowNewSessionEntry(bool showNewSessionEntry)
{
    if (!canStartNewSession()) {
        return;
    }

    if (showNewSessionEntry == m_showNewSessionEntry) {
        return;
    }

    int row = m_data.size();
    if (showNewSessionEntry) {
        beginInsertRows(QModelIndex(), row, row);
        m_showNewSessionEntry = showNewSessionEntry;
        endInsertRows();
    } else {
        beginRemoveRows(QModelIndex(), row, row);
        m_showNewSessionEntry = showNewSessionEntry;
        endRemoveRows();
    }
    Q_EMIT countChanged();
}

QVariant SessionsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() > rowCount(QModelIndex())) {
        return QVariant();
    }

    if (index.row() == m_data.count()) {
        switch (role) {
        case RealNameRole:
            return i18n("New Session");
        case IconNameRole:
            return QStringLiteral("system-switch-user");
        case NameRole:
            return i18n("New Session");
        case DisplayNumberRole:
            return 0; // NA
        case VtNumberRole:
            return -1; // an invalid VtNumber - which we'll use to indicate it's to start a new session
        case SessionRole:
            return 0; // NA
        case IsTtyRole:
            return false; // NA
        default:
            return QVariant();
        }
    }

    const SessionEntry &item = m_data.at(index.row());

    switch (role) {
    case RealNameRole:
        return item.realName;
    case IconRole:
        return item.icon;
    case NameRole:
        return item.name;
    case DisplayNumberRole:
        return item.displayNumber;
    case VtNumberRole:
        return item.vtNumber;
    case SessionRole:
        return item.session;
    case IsTtyRole:
        return item.isTty;
    default:
        return QVariant();
    }
}

int SessionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.count() + (m_showNewSessionEntry ? 1 : 0);
}

QHash<int, QByteArray> SessionsModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;

    roleNames[NameRole] = QByteArrayLiteral("name");
    roleNames[RealNameRole] = QByteArrayLiteral("realName");
    roleNames[IconRole] = QByteArrayLiteral("icon");
    roleNames[IconNameRole] = QByteArrayLiteral("iconName");
    roleNames[DisplayNumberRole] = QByteArrayLiteral("displayNumber");
    roleNames[VtNumberRole] = QByteArrayLiteral("vtNumber");
    roleNames[SessionRole] = QByteArrayLiteral("session");
    roleNames[IsTtyRole] = QByteArrayLiteral("isTty");

    return roleNames;
}
