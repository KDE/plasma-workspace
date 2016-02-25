/*
    Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include "sessionsmodel.h"

#include <KAuthorized>
#include <KUser>

#include "kscreensaversettings.h"

#include "screensaver_interface.h"

SessionsModel::SessionsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_screensaverInterface(
        new org::freedesktop::ScreenSaver(QStringLiteral("org.freedesktop.ScreenSaver"),
                                          QStringLiteral("/ScreenSaver"),
                                          QDBusConnection::sessionBus(),
                                          this)
        )
{
    reload();

    // wait for the screen locker to be ready before actually switching
    connect(m_screensaverInterface, &org::freedesktop::ScreenSaver::ActiveChanged, this, [this](bool active) {
        if (active) {
            if (m_pendingVt) {
                m_displayManager.switchVT(m_pendingVt);
                emit switchedUser(m_pendingVt);
            } else if (m_pendingReserve) {
                m_displayManager.startReserve();
                emit startNewSession();
            }

            m_pendingVt = 0;
            m_pendingReserve = false;
        }
    });
}

bool SessionsModel::canSwitchUser() const
{
    return const_cast<SessionsModel *>(this)->m_displayManager.isSwitchable() && KAuthorized::authorizeKAction(QLatin1String("switch_user"));
}

bool SessionsModel::canStartNewSession() const
{
    return const_cast<SessionsModel *>(this)->m_displayManager.numReserve() > 0 && KAuthorized::authorizeKAction(QLatin1String("start_new_session"));
}

bool SessionsModel::shouldLock() const
{
    return m_shouldLock;
}

void SessionsModel::switchUser(int vt, bool shouldLock)
{
    if (!canSwitchUser() || vt <= 0) {
        return;
    }

    if (!shouldLock) {
        m_displayManager.switchVT(vt);
        emit switchedUser(vt);
        return;
    }

    checkScreenLocked([this, vt](bool locked) {
        if (locked) {
            // already locked, switch right away
            m_displayManager.switchVT(vt);
            emit switchedUser(vt);
        } else {
            m_pendingReserve = false;
            m_pendingVt = vt;
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
        emit startedNewSession();
        return;
    }

    checkScreenLocked([this](bool locked) {
        if (locked) {
            // already locked, switch right away
            m_displayManager.startReserve();
            emit startedNewSession();
        } else {
            m_pendingReserve = true;
            m_pendingVt = 0;
            m_screensaverInterface->Lock();
        }
    });
}

void SessionsModel::reload()
{
    static QHash<QString, KUser> kusers;

    const bool oldShouldLock = m_shouldLock;
    m_shouldLock = KAuthorized::authorizeKAction(QStringLiteral("lock_screen")) && KScreenSaverSettings::autolock();
    if (m_shouldLock != oldShouldLock) {
        emit shouldLockChanged();
    }

    SessList sessions;
    m_displayManager.localSessions(sessions);

    const int oldCount = m_data.count();

    beginResetModel();

    m_data.clear();
    m_data.reserve(sessions.count());

    foreach (const SessEnt &session, sessions) {
        if (!session.vt || session.self) {
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
        emit countChanged();
    }
}

void SessionsModel::checkScreenLocked(const std::function<void (bool)> &cb)
{
    auto reply = m_screensaverInterface->GetActive();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, cb](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<bool> reply = *watcher;
        if (!reply.isError()) {
            cb(reply.value());
        }
        watcher->deleteLater();
    });
}

QVariant SessionsModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_data.count()) {
        return QVariant();
    }

    const SessionEntry &item = m_data.at(index.row());

    switch (static_cast<Role>(role)) {
    case Role::RealName: return item.realName;
    case Role::Icon: return item.icon;
    case Role::Name: return item.name;
    case Role::DisplayNumber: return item.displayNumber;
    case Role::VtNumber: return item.vtNumber;
    case Role::Session: return item.session;
    case Role::IsTty: return item.isTty;
    }

    return QVariant();
}

int SessionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.count();
}

QHash<int, QByteArray> SessionsModel::roleNames() const
{
    return {
        {static_cast<int>(Role::Name), QByteArrayLiteral("name")},
        {static_cast<int>(Role::RealName), QByteArrayLiteral("realName")},
        {static_cast<int>(Role::Icon), QByteArrayLiteral("icon")},
        {static_cast<int>(Role::DisplayNumber), QByteArrayLiteral("displayNumber")},
        {static_cast<int>(Role::VtNumber), QByteArrayLiteral("vtNumber")},
        {static_cast<int>(Role::Session), QByteArrayLiteral("session")},
        {static_cast<int>(Role::IsTty), QByteArrayLiteral("isTty")}
    };
}
