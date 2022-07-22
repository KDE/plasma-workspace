/*
    SPDX-FileCopyrightText: 1997 Matthias Kalle Dalheimer <kalle@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kworkspace.h"
#include "config-libkworkspace.h"

#include <QDBusConnection>
#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QSocketNotifier>
#include <QTextStream>
#include <ksmserver_interface.h>
#include <stdlib.h> // getenv()

#if HAVE_X11
#include <X11/SM/SMlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <fixx11h.h>
#endif

#include "config-workspace.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#include <pwd.h>
#include <sys/types.h>

namespace KWorkSpace
{
void requestShutDown(ShutdownConfirm confirm, ShutdownType sdtype, ShutdownMode sdmode)
{
    org::kde::KSMServerInterface ksmserver(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
    ksmserver.logout((int)confirm, (int)sdtype, (int)sdmode);
}

bool canShutDown(ShutdownConfirm confirm, ShutdownType sdtype, ShutdownMode sdmode)
{
#if HAVE_X11
    if (confirm == ShutdownConfirmYes || sdtype != ShutdownTypeDefault || sdmode != ShutdownModeDefault) {
        org::kde::KSMServerInterface ksmserver(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
        QDBusReply<bool> reply = ksmserver.canShutdown();
        if (!reply.isValid()) {
            return false;
        }
        return reply;
    }

    return true;
#else
    return false;
#endif
}

bool isShuttingDown()
{
    org::kde::KSMServerInterface ksmserver(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
    QDBusReply<bool> reply = ksmserver.isShuttingDown();
    if (!reply.isValid()) {
        return false;
    }
    return reply;
}

static QTime smModificationTime;
void propagateSessionManager()
{
#if HAVE_X11
    QByteArray fName = QFile::encodeName(QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + "/KSMserver");
    QString display = QString::fromLocal8Bit(::getenv("DISPLAY"));
    // strip the screen number from the display
    display.remove(QRegularExpression(QStringLiteral("\\.\\d+$")));
    int i;
    while ((i = display.indexOf(QLatin1Char(':'))) >= 0)
        display[i] = '_';
    while ((i = display.indexOf(QLatin1Char('/'))) >= 0)
        display[i] = '_';

    fName += '_';
    fName += display.toLocal8Bit();
    QByteArray smEnv = ::getenv("SESSION_MANAGER");
    bool check = smEnv.isEmpty();
    if (!check && smModificationTime.isValid()) {
        QFileInfo info(fName);
        QTime current = info.lastModified().time();
        check = current > smModificationTime;
    }
    if (check) {
        QFile f(fName);
        if (!f.open(QIODevice::ReadOnly))
            return;
        QFileInfo info(f);
        smModificationTime = QTime(info.lastModified().time());
        ::setenv("SESSION_MANAGER", f.readLine().trimmed(), true);
    }
#endif
}

void detectPlatform(int argc, char **argv)
{
    if (qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        return;
    }
    for (int i = 0; i < argc; i++) {
        if (qstrcmp(argv[i], "-platform") == 0 || qstrcmp(argv[i], "--platform") == 0 || QByteArray(argv[i]).startsWith("-platform=")
            || QByteArray(argv[i]).startsWith("--platform=")) {
            return;
        }
    }
    const QByteArray sessionType = qgetenv("XDG_SESSION_TYPE");
    if (sessionType.isEmpty()) {
        return;
    }
    if (qstrcmp(sessionType, "wayland") == 0) {
        qputenv("QT_QPA_PLATFORM", "wayland");
    } else if (qstrcmp(sessionType, "x11") == 0) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
}

} // end namespace
