/*
    SPDX-FileCopyrightText: 2008 Dario Freddi <drf@kdemod.ath.cx>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KLocalizedString>
#include <KRunner/AbstractRunner>
#include <QDBusConnection>

class SessionManagement;

struct RunnerKeyword {
    inline RunnerKeyword(KLocalizedString str, bool partialMatch = true)
        : triggerWord(QString::fromUtf8(str.untranslatedText()))
        , translatedTriggerWord(str.toString())
        , supportPartialMatch(partialMatch)
    {
    }
    const QString triggerWord;
    const QString translatedTriggerWord;
    const bool supportPartialMatch = true;
};

class PowerDevilRunner : public KRunner::AbstractRunner
{
    Q_OBJECT
public:
    PowerDevilRunner(QObject *parent, const KPluginMetaData &metaData);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &action) override;

private:
    void updateSyntaxes();
    void addSuspendMatch(int value, QList<KRunner::QueryMatch> &matches, KRunner::QueryMatch::CategoryRelevance categoryRelevance);
    // Returns -1 if there is no match, otherwise the percentage that the user has entered
    int matchesScreenBrightnessKeywords(const QString &query) const;
    bool matchesRunnerKeywords(const QList<RunnerKeyword> &keywords, KRunner::QueryMatch::CategoryRelevance &categoryRelevance, const QString &query) const;
    void addSyntaxForKeyword(const QList<RunnerKeyword> &keywords, const QString &description);

    SessionManagement *m_session;
    const RunnerKeyword m_power;
    const RunnerKeyword m_suspend, m_toRam;
    const RunnerKeyword m_sleep;
    const RunnerKeyword m_hibernate, m_toDisk;
    const RunnerKeyword m_hybridSuspend, m_hybrid;
    const RunnerKeyword m_dimScreen;
    const RunnerKeyword m_screenBrightness;
};
