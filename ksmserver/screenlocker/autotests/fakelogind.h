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
#ifndef FAKELOGIND_H
#define FAKELOGIND_H

#include <QObject>
#include <QtDBus/QtDBus>

class FakeLogindSession;

class FakeLogind : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.login1.Manager")
public:
    explicit FakeLogind(QObject *parent = nullptr);
    virtual ~FakeLogind();

    FakeLogindSession *session() const {
        return m_session;
    }

public Q_SLOTS:
    Q_SCRIPTABLE QDBusObjectPath GetSessionByPID(quint32 pid);
    Q_SCRIPTABLE void lock();
    Q_SCRIPTABLE void unlock();

private:
    FakeLogindSession *m_session;
};

class FakeLogindSession : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.login1.Session")
public:
    explicit FakeLogindSession(const QString &path, QObject *parent = nullptr);
    virtual ~FakeLogindSession();

    const QString &path() {
        return m_path;
    }

    void lock();
    void unlock();

Q_SIGNALS:
    Q_SCRIPTABLE void Lock();
    Q_SCRIPTABLE void Unlock();

private:
    QString m_path;
};

#endif
