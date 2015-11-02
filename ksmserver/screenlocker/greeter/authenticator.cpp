/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 1999 Martin R. Jones <mjones@kde.org>
Copyright (C) 2002 Luboš Luňák <l.lunak@kde.org>
Copyright (C) 2003 Oswald Buddenhagen <ossi@kde.org>
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
#include "authenticator.h"
#include <kcheckpass-enums.h>
#include <config-kscreenlocker.h>

// Qt
#include <QFile>
#include <QSocketNotifier>
#include <QTimer>

// system
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

Authenticator::Authenticator(QObject *parent)
    : QObject(parent)
    , m_graceLockTimer(new QTimer(this))
{
    m_graceLockTimer->setSingleShot(true);
    m_graceLockTimer->setInterval(3000);
    connect(m_graceLockTimer, &QTimer::timeout, this, &Authenticator::graceLockedChanged);
}

Authenticator::~Authenticator() = default;

void Authenticator::tryUnlock(const QString &password)
{
    if (isGraceLocked()) {
        emit failed();
        return;
    }
    m_graceLockTimer->start();
    emit graceLockedChanged();

    KCheckPass *checkPass = new KCheckPass(password, this);
    connect(checkPass, &KCheckPass::succeeded, this, &Authenticator::succeeded);
    connect(checkPass, &KCheckPass::failed, this, &Authenticator::failed);
    connect(checkPass, &KCheckPass::message, this, &Authenticator::message);
    connect(checkPass, &KCheckPass::error, this, &Authenticator::error);
    checkPass->start();
}

bool Authenticator::isGraceLocked() const
{
    return m_graceLockTimer->isActive();
}

KCheckPass::KCheckPass(const QString &password, QObject *parent)
    : QObject(parent)
    , m_password(password)
    , m_notifier(nullptr)
    , m_pid(0)
    , m_fd(0)
{
    connect(this, &KCheckPass::succeeded, this, &QObject::deleteLater);
    connect(this, &KCheckPass::failed, this, &QObject::deleteLater);
}

KCheckPass::~KCheckPass() = default;

void KCheckPass::start()
{
    int sfd[2];
    char fdbuf[16];

    if (m_notifier)
        return;
    if (::socketpair(AF_LOCAL, SOCK_STREAM, 0, sfd)) {
        cantCheck();
        return;
    }
    if ((m_pid = ::fork()) < 0) {
        ::close(sfd[0]);
        ::close(sfd[1]);
        cantCheck();
        return;
    }
    if (!m_pid) {
        ::close(sfd[0]);
        sprintf(fdbuf, "%d", sfd[1]);
        execlp(QFile::encodeName(QStringLiteral(KCHECKPASS_BIN)).data(),
               "kcheckpass",
               "-m", "classic",
               "-S", fdbuf,
               (char *)0);
        _exit(20);
    }
    ::close(sfd[1]);
    m_fd = sfd[0];
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &KCheckPass::handleVerify);
}

////// kckeckpass interface code

int KCheckPass::Reader(void *buf, int count)
{
    int ret, rlen;

    for (rlen = 0; rlen < count; ) {
      dord:
        ret = ::read(m_fd, (void *)((char *)buf + rlen), count - rlen);
        if (ret < 0) {
            if (errno == EINTR)
                goto dord;
            if (errno == EAGAIN)
                break;
            return -1;
        }
        if (!ret)
            break;
        rlen += ret;
    }
    return rlen;
}

bool KCheckPass::GRead(void *buf, int count)
{
    return Reader(buf, count) == count;
}

bool KCheckPass::GWrite(const void *buf, int count)
{
    return ::write(m_fd, buf, count) == count;
}

bool KCheckPass::GSendInt(int val)
{
    return GWrite(&val, sizeof(val));
}

bool KCheckPass::GSendStr(const char *buf)
{
    int len = buf ? ::strlen (buf) + 1 : 0;
    return GWrite(&len, sizeof(len)) && GWrite (buf, len);
}

bool KCheckPass::GSendArr(int len, const char *buf)
{
    return GWrite(&len, sizeof(len)) && GWrite (buf, len);
}

bool KCheckPass::GRecvInt(int *val)
{
    return GRead(val, sizeof(*val));
}

bool KCheckPass::GRecvArr(char **ret)
{
    int len;
    char *buf;

    if (!GRecvInt(&len))
        return false;
    if (!len) {
        *ret = 0;
        return true;
    }
    if (!(buf = (char *)::malloc (len)))
        return false;
    *ret = buf;
    if (GRead (buf, len)) {
        return true;
    } else {
        ::free(buf);
        *ret = 0;
        return false;
    }
}

void KCheckPass::handleVerify()
{
    int ret;
    char *arr;

    if (GRecvInt( &ret )) {
        switch (ret) {
        case ConvGetBinary:
            if (!GRecvArr( &arr ))
                break;
            // FIXME: not supported
            cantCheck();
            if (arr)
                ::free( arr );
            return;
        case ConvGetNormal:
            if (!GRecvArr( &arr ))
                break;
            GSendStr(m_password.toUtf8().constData());
            if (!m_password.isEmpty()) {
                // IsSecret
                GSendInt(1);
            }
            if (arr)
                ::free( arr );
            return;
        case ConvGetHidden:
            if (!GRecvArr( &arr ))
                break;
            GSendStr(m_password.toUtf8().constData());
            if (!m_password.isEmpty()) {
                // IsSecret
                GSendInt(1);
            }
            if (arr)
                ::free( arr );
            return;
        case ConvPutInfo:
            if (!GRecvArr( &arr ))
                break;
            emit message(QString::fromLocal8Bit(arr));
            ::free( arr );
            return;
        case ConvPutError:
            if (!GRecvArr( &arr ))
                break;
            emit error(QString::fromLocal8Bit(arr));
            ::free( arr );
            return;
        }
    }
    reapVerify();
}

void KCheckPass::reapVerify()
{
    m_notifier->setEnabled( false );
    m_notifier->deleteLater();
    m_notifier = nullptr;
    ::close( m_fd );
    int status;
    while (::waitpid( m_pid, &status, 0 ) < 0)
        if (errno != EINTR) { // This should not happen ...
            cantCheck();
            return;
        }
    if (WIFEXITED(status))
        switch (WEXITSTATUS(status)) {
        case AuthOk:
            emit succeeded();
            return;
        case AuthBad:
            emit failed();
            return;
        case AuthAbort:
            return;
        }
    cantCheck();
}

void KCheckPass::cantCheck()
{
    // TODO: better signal?
    emit failed();
}
