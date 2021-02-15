/*
   Copyright (C) 2021 David Edmundson <davidedmundson@kde.org>

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

#pragma once

#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QEventLoopLocker>


namespace DBusUtitls {
   /**
    * A lot of DBus calls are used as fire-and-forget.
    * This utility function prints a warning if the call receives and error.
    *
    * It also holds an EventLoopLocker whilst any call is in flight.
    * This is useful for:
    *   - https://gitlab.freedesktop.org/dbus/dbus/-/issues/72 in which a call may not be dispatched to a DBus activated app if the caller quits before the message is forwarded
    *   - logind methods, which query information from the caller on reciept
    */
    template <class T>
    static void watchCall(const T &call) {
        auto watcher = new QDBusPendingCallWatcher(call);

        // will be released when the lambda is destroyed, which happens implicitly with the watcher
        auto lock = std::make_unique<QEventLoopLocker>();
        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [lock = std::move(lock), watcher, call]() {
            if (call.isError()) {
                qCritical() << "Session management call failed:" << call.error().message();
            }
            watcher->deleteLater();
        });
    }
}
