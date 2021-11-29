/*
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: MIT
*/

#include "sessionsmodel.h"

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
    return const_cast<SessionsModel *>(this)->m_displayManager.isSwitchable() && KAuthorized::authorizeAction(QLatin1String("switch_user"));
}

bool SessionsModel::canStartNewSession() const
{
    return const_cast<SessionsModel *>(this)->m_displayManager.numReserve() > 0 && KAuthorized::authorizeAction(QLatin1String("start_new_session"));
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

    Q_FOREACH (const SessEnt &session, sessions) {
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
        switch (static_cast<Role>(role)) {
        case Role::RealName:
            return i18n("New Session");
        case Role::IconName:
            return QStringLiteral("system-switch-user");
        case Role::Name:
            return i18n("New Session");
        case Role::DisplayNumber:
            return 0; // NA
        case Role::VtNumber:
            return -1; // an invalid VtNumber - which we'll use to indicate it's to start a new session
        case Role::Session:
            return 0; // NA
        case Role::IsTty:
            return false; // NA
        default:
            return QVariant();
        }
    }

    const SessionEntry &item = m_data.at(index.row());

    switch (static_cast<Role>(role)) {
    case Role::RealName:
        return item.realName;
    case Role::Icon:
        return item.icon;
    case Role::Name:
        return item.name;
    case Role::DisplayNumber:
        return item.displayNumber;
    case Role::VtNumber:
        return item.vtNumber;
    case Role::Session:
        return item.session;
    case Role::IsTty:
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
    return {
        {static_cast<int>(Role::Name), QByteArrayLiteral("name")},
        {static_cast<int>(Role::RealName), QByteArrayLiteral("realName")},
        {static_cast<int>(Role::Icon), QByteArrayLiteral("icon")},
        {static_cast<int>(Role::IconName), QByteArrayLiteral("iconName")},
        {static_cast<int>(Role::DisplayNumber), QByteArrayLiteral("displayNumber")},
        {static_cast<int>(Role::VtNumber), QByteArrayLiteral("vtNumber")},
        {static_cast<int>(Role::Session), QByteArrayLiteral("session")},
        {static_cast<int>(Role::IsTty), QByteArrayLiteral("isTty")},
    };
}
