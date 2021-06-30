/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>

    SPDX-License-Identifier: MIT
*/

#pragma once

// needed to avoid clash with INT8 defined in X11/Xmd.h on solaris
#define QT_CLEAN_NAMESPACE 1

#include <kworkspace.h>

#include "server.h"

class KSMClient
{
public:
    explicit KSMClient(SmsConn);
    ~KSMClient();

    void registerClient(const char *previousId = nullptr);
    SmsConn connection() const
    {
        return smsConn;
    }

    void resetState();
    uint saveYourselfDone : 1;
    uint pendingInteraction : 1;
    uint waitForPhase2 : 1;
    uint wasPhase2 : 1;

    QList<SmProp *> properties;
    SmProp *property(const char *name) const;

    QString program() const;
    QStringList restartCommand() const;
    QStringList discardCommand() const;
    int restartStyleHint() const;
    QString userId() const;
    const char *clientId()
    {
        return id ? id : "";
    }

private:
    const char *id;
    SmsConn smsConn;
};