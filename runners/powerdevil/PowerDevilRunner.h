/*
    SPDX-FileCopyrightText: 2008 Dario Freddi <drf@kdemod.ath.cx>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KRunner/AbstractRunner>
#include <QDBusConnection>

class SessionManagement;

struct RunnerKeyword {
    QString triggerWord;
    QString translatedTriggerWord;
    bool supportPartialMatch = true;
};

class PowerDevilRunner : public KRunner::AbstractRunner
{
    Q_OBJECT
public:
    PowerDevilRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

private Q_SLOTS:
    void updateStatus();

private:
    void updateSyntaxes();
    void addSuspendMatch(int value, QList<KRunner::QueryMatch> &matches, KRunner::QueryMatch::Type type);
    // Returns -1 if there is no match, otherwise the percentage that the user has entered
    int matchesScreenBrightnessKeywords(const QString &query) const;
    bool matchesRunnerKeywords(const QList<RunnerKeyword> &keywords, KRunner::QueryMatch::Type &type, const QString &query) const;
    void addSyntaxForKeyword(const QList<RunnerKeyword> &keywords, const QString &description);

    SessionManagement *m_session;
    RunnerKeyword m_suspend, m_toRam;
    RunnerKeyword m_sleep;
    RunnerKeyword m_hibernate, m_toDisk;
    RunnerKeyword m_dimScreen;
    RunnerKeyword m_screenBrightness;
    RunnerKeyword m_turnOffScreen;
};
