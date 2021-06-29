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

class PowerDevilRunner : public Plasma::AbstractRunner
{
    Q_OBJECT
public:
    PowerDevilRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args);
    ~PowerDevilRunner() override;

    void match(Plasma::RunnerContext &context) override;
    void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

private Q_SLOTS:
    void updateStatus();

private:
    void updateSyntaxes();
    void addSuspendMatch(int value, QList<Plasma::QueryMatch> &matches, Plasma::QueryMatch::Type type);
    // Returns -1 if there is no match, otherwise the percentage that the user has entered
    int matchesScreenBrightnessKeywords(const QString &query) const;
    bool matchesRunnerKeywords(const QList<RunnerKeyword> &keywords, Plasma::QueryMatch::Type &type, const QString &query) const;
    void addSyntaxForKeyword(const QList<RunnerKeyword> &keywords, const QString &description);

    SessionManagement *m_session;
    RunnerKeyword m_suspend, m_toRam;
    RunnerKeyword m_sleep;
    RunnerKeyword m_hibernate, m_toDisk;
    RunnerKeyword m_dimScreen;
    RunnerKeyword m_screenBrightness;
};
