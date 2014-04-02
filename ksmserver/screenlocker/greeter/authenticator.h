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
#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include <QObject>

class QSocketNotifier;
class QTimer;

class Authenticator : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool graceLocked READ isGraceLocked NOTIFY graceLockedChanged)
public:
    explicit Authenticator(QObject *parent = nullptr);
    ~Authenticator();

    bool isGraceLocked() const;

public Q_SLOTS:
    void tryUnlock(const QString &password);

Q_SIGNALS:
    void failed();
    void succeeded();
    void graceLockedChanged();
    void message(const QString &);
    void error(const QString &);

private:
    QTimer *m_graceLockTimer;
};

class KCheckPass : public QObject
{
    Q_OBJECT
public:
    explicit KCheckPass(const QString &password, QObject *parent = nullptr);
    ~KCheckPass();

    void start();

Q_SIGNALS:
    void failed();
    void succeeded();
    void message(const QString &);
    void error(const QString &);

private Q_SLOTS:
    void handleVerify();

private:
    void cantCheck();
    void reapVerify();
    // kcheckpass interface
    int Reader(void *buf, int count);
    bool GRead(void *buf, int count);
    bool GWrite(const void *buf, int count);
    bool GSendInt(int val);
    bool GSendStr(const char *buf);
    bool GSendArr(int len, const char *buf);
    bool GRecvInt(int *val);
    bool GRecvArr(char **buf);

    QString m_password;
    QSocketNotifier *m_notifier;
    int m_pid;
    int m_fd;
};

#endif
