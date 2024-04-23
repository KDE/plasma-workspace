/*
    SPDX-FileCopyrightText: 2017 Valerio Pilo <vpilo@coldshock.net>

    SPDX-License-Identifier: LGPL-2.0-only
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
    void unregistered();
    void timeout();

private:
    constexpr static const int dbusTimeoutSec = 60;

    QString mService = QStringLiteral("org.freedesktop.Notifications");
    QTimer mTimeoutTimer;
    enum Mode {
        WaitForRegistration,
        WaitForUnregistration,
    } mMode = WaitForRegistration;
};
