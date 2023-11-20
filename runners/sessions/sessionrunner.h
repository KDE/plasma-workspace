/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include <kdisplaymanager.h>
#include <krunner/abstractrunner.h>

#include <sessionmanagement.h>

/**
 * This class provides matches for running sessions as well as
 * an action to start a new session, etc.
 */
class SessionRunner : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    SessionRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

    enum {
        LogoutAction = 1,
        ShutdownAction,
        RestartAction,
        LockAction,
        SaveAction,
    };

private:
    void matchCommands(QList<KRunner::QueryMatch> &matches, const QString &term);

    QStringList m_logoutKeywords;
    QStringList m_shutdownKeywords;
    QStringList m_restartKeywords;
    QStringList m_lockKeywords;
    QStringList m_saveKeywords;
    QStringList m_usersKeywords;
    QString m_sessionsKeyword;
    QString m_switchKeyword;
    SessionManagement m_session;
};
