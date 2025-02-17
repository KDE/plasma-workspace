/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <QDBusContext>
#include <QDBusMessage>
#include <QObject>

#include <KRunner/QueryMatch>

#include <kdisplaymanager.h>
#include <sessionmanagement.h>

#include "dbusutils_p.h"

/**
 * This class provides matches for running sessions as well as
 * an action to start a new session, etc.
 */
class SessionRunner : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    SessionRunner(QObject *parent = nullptr);

    RemoteActions Actions();
    RemoteMatches Match(const QString &term);
    void Run(const QString &id, const QString &actionId);

    enum Action {
        LogoutAction = 1,
        ShutdownAction,
        RestartAction,
        LockAction,
        SaveAction,
        SwitchUserAction
    };

private:
    RemoteMatches matchCommands(const QString &term);

    QStringList m_logoutKeywords;
    QStringList m_shutdownKeywords;
    QStringList m_restartKeywords;
    QStringList m_lockKeywords;
    QStringList m_saveKeywords;
    QStringList m_usersKeywords;
    QString m_sessionsKeyword;
    QString m_switchKeyword;
    KDisplayManager dm;
    SessionManagement m_session;
};
