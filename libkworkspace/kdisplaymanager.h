/*
   Copyright (C) 2004,2005 Oswald Buddenhagen <ossi@kde.org>
   Copyright (C) 2005 Stephan Kulow <coolo@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDISPLAYMANAGER_H
#define KDISPLAYMANAGER_H

#include "kworkspace.h"
#include "config-libkworkspace.h"
#include "kworkspace_export.h"
#include <QString>
#include <QList>
#include <QByteArray>

struct KWORKSPACE_EXPORT SessEnt {
    QString display, from, user, session;
    int vt;
    bool self:1, tty:1;
};

typedef QList<SessEnt> SessList;

class KWORKSPACE_EXPORT KDisplayManager {

#if HAVE_X11

public:
    KDisplayManager();
    ~KDisplayManager();

    bool canShutdown();
    void shutdown(KWorkSpace::ShutdownType shutdownType,
                  KWorkSpace::ShutdownMode shutdownMode,
                  const QString &bootOption = QString());

    bool isSwitchable();
    int numReserve();
    void startReserve();
    bool localSessions(SessList &list);
    bool switchVT(int vt);
    void lockSwitchVT(int vt);

    bool bootOptions(QStringList &opts, int &dflt, int &curr);

    static QString sess2Str(const SessEnt &se);
    static void sess2Str2(const SessEnt &se, QString &user, QString &loc);

private:
    bool exec(const char *cmd, QByteArray &ret);
    bool exec(const char *cmd);

    void GDMAuthenticate();

#else // Q_WS_X11

public:
    KDisplayManager() {}

    bool canShutdown() { return false; }
    void shutdown(KWorkSpace::ShutdownType shutdownType,
                  KWorkSpace::ShutdownMode shutdownMode,
                  const QString &bootOption = QString()) {}

    void setLock(bool) {}

    bool isSwitchable() { return false; }
    int numReserve() { return -1; }
    void startReserve() {}
    bool localSessions(SessList &list) { return false; }
    void switchVT(int vt) {}

    bool bootOptions(QStringList &opts, int &dflt, int &curr);

#endif // HAVE_X11

private:
#if HAVE_X11
    class Private;
    Private * const d;
#endif // HAVE_X11

}; // class KDisplayManager

#endif // KDISPLAYMANAGER_H

