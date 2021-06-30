/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2018 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

#include <QObject>
#include <kworkspace.h>

class Shutdown : public QObject
{
    Q_OBJECT
public:
    Shutdown(QObject *parent = nullptr);
    void logout();
    void logoutAndShutdown();
    void logoutAndReboot();
private Q_SLOTS:
    void logoutCancelled();
    void logoutComplete();

private:
    void startLogout(KWorkSpace::ShutdownType shutdownType);
    void runShutdownScripts();
    KWorkSpace::ShutdownType m_shutdownType;
};
