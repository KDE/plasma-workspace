/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "fakelogind.h"

FakeLogind::FakeLogind(QObject *parent)
    : QObject(parent)
    , m_session(new FakeLogindSession(QStringLiteral("/org/freedesktop/login1/session/_1"), this))
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/org/freedesktop/login1"), this, QDBusConnection::ExportScriptableContents);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.freedesktop.login1"));
}

FakeLogind::~FakeLogind()
{
    QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/org/freedesktop/login1"));
    QDBusConnection::sessionBus().unregisterService(QStringLiteral("org.freedesktop.login1"));
}

QDBusObjectPath FakeLogind::GetSessionByPID(quint32 pid)
{
    Q_UNUSED(pid)
    return QDBusObjectPath(m_session->path());
}

void FakeLogind::lock()
{
    m_session->lock();
}

void FakeLogind::unlock()
{
    m_session->unlock();
}

FakeLogindSession::FakeLogindSession(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
{
    QDBusConnection::sessionBus().registerObject(m_path, this, QDBusConnection::ExportScriptableContents);
}

FakeLogindSession::~FakeLogindSession()
{
    QDBusConnection::sessionBus().unregisterObject(m_path);
}

void FakeLogindSession::lock()
{
    emit Lock();
}

void FakeLogindSession::unlock()
{
    emit Unlock();
}
