/*
 *   Copyright © 2017 Valerio Pilo <vpilo@coldshock.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#pragma once

#include <QCoreApplication>
#include <QString>
#include <QTimer>

class Waiter : public QCoreApplication
{
    Q_OBJECT

public:
    Waiter(int argc, char **argv);
    bool waitForService();

private Q_SLOTS:
    void registered();
    void timeout();

private:
    constexpr static const int dbusTimeoutSec = 60;

    QString mService = QStringLiteral("org.freedesktop.Notifications");
    QTimer mTimeoutTimer;
};
