/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2018 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QObject>

enum ShutdownType {
    ShutdownTypeNone = 0,
    ShutdownTypeReboot = 1,
    ShutdownTypeHalt = 2,
    ShutdownTypeLogout = 3,
};

class Shutdown : public QObject
{
    Q_OBJECT
public:
    Shutdown(QObject *parent = nullptr);
    void logout();
    void logoutAndShutdown();
    void logoutAndReboot();
    void saveSession();
private Q_SLOTS:
    void logoutCancelled();
    void logoutComplete();
    void ksmServerComplete();

private:
    void startLogout(ShutdownType shutdownType);
    void runShutdownScripts();
    bool usingSystemdManagedSession();
    // ShutdownTypeNone means idle, see startLogout().
    ShutdownType m_shutdownType = ShutdownTypeNone;
};
