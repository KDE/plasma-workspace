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
class SessionRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

public:
    SessionRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~SessionRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

    enum {
        LogoutAction = 1,
        ShutdownAction,
        RestartAction,
        LockAction,
    };

private:
    void matchCommands(QList<Plasma::QueryMatch> &matches, const QString &term);

    QString m_triggerWord;
    KDisplayManager dm;
    SessionManagement m_session;
    bool m_canLogout;
};
