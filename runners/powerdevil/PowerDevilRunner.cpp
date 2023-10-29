/*
    SPDX-FileCopyrightText: 2008 Dario Freddi <drf@kdemod.ath.cx>
    SPDX-FileCopyrightText: 2008 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2023 Natalie Clarius <natalie_clarius@yahoo.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "PowerDevilRunner.h"

#include <sessionmanagement.h>

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QIcon>

#include <KConfig>
#include <KConfigGroup>

#include <cmath>

K_PLUGIN_CLASS_WITH_JSON(PowerDevilRunner, "plasma-runner-powerdevil.json")

PowerDevilRunner::PowerDevilRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
    , m_session(new SessionManagement(this))
    , m_power(ki18nc("Note this is a KRunner keyword; 'power' as in 'power saving mode'", "power"))
    , m_suspend(ki18nc("Note this is a KRunner keyword", "suspend"))
    , m_toRam(ki18nc("Note this is a KRunner keyword", "to ram"), false)
    , m_sleep(ki18nc("Note this is a KRunner keyword", "sleep"))
    , m_hibernate(ki18nc("Note this is a KRunner keyword", "hibernate"))
    , m_toDisk(ki18nc("Note this is a KRunner keyword", "to disk"), false)
    , m_hybridSuspend(ki18nc("Note this is a KRunner keyword", "hybrid sleep"), false)
    , m_hybrid(ki18nc("Note this is a KRunner keyword", "hybrid"), false)
    , m_dimScreen(ki18nc("Note this is a KRunner keyword", "dim screen"))
    , m_screenBrightness(ki18nc("Note this is a KRunner keyword", "dim screen"))
{
    setMinLetterCount(3);
    updateSyntaxes();
}

void PowerDevilRunner::updateSyntaxes()
{
    setSyntaxes({}); // Clear the existing ones
    addSyntaxForKeyword({m_suspend},
                        i18n("Lists system suspend (e.g. sleep, hibernate) options "
                             "and allows them to be activated"));

    if (m_session->canSuspend()) {
        addSyntaxForKeyword({m_sleep, m_toRam}, i18n("Suspends the system to RAM"));
    }

    if (m_session->canHibernate()) {
        addSyntaxForKeyword({m_hibernate, m_toDisk}, i18n("Suspends the system to disk"));
    }

    if (m_session->canHybridSuspend()) {
        addSyntaxForKeyword({m_hybrid, m_hybridSuspend}, i18n("Sleeps now and falls back to hibernate"));
    }

    addSyntax(QStringList{i18nc("Note this is a KRunner keyword, <> is a placeholder and should be at the end", "screen brightness <percent value>"),
                          i18nc("Note this is a KRunner keyword", "dim screen")},
              // xgettext:no-c-format
              i18n("Lists screen brightness options or sets it to the brightness defined by the search term; "
                   "e.g. screen brightness 50 would dim the screen to 50% maximum brightness"));
}

enum SleepState { SuspendState, HibernateState, HybridSuspendState };

void PowerDevilRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();
    KRunner::QueryMatch::Type type = KRunner::QueryMatch::ExactMatch;
    QList<KRunner::QueryMatch> matches;

    int screenBrightnessResults = matchesScreenBrightnessKeywords(term);
    if (screenBrightnessResults != -1) {
        KRunner::QueryMatch match(this);
        match.setType(type);
        match.setIconName(QStringLiteral("video-display-brightness"));
        match.setText(i18n("Set Brightness to %1%", screenBrightnessResults));
        match.setData(screenBrightnessResults);
        match.setRelevance(1);
        match.setId(QStringLiteral("BrightnessChange"));
        matches.append(match);
    } else if (matchesRunnerKeywords({m_screenBrightness, m_dimScreen}, type, term)) {
        KRunner::QueryMatch match1(this);
        match1.setType(KRunner::QueryMatch::ExactMatch);
        match1.setIconName(QStringLiteral("video-display-brightness"));
        match1.setText(i18n("Dim screen totally"));
        match1.setRelevance(1);
        match1.setId(QStringLiteral("DimTotal"));
        matches.append(match1);

        KRunner::QueryMatch match2(this);
        match2.setType(type);
        match2.setIconName(QStringLiteral("video-display-brightness"));
        match2.setText(i18n("Dim screen by half"));
        match2.setRelevance(1);
        match2.setId(QStringLiteral("DimHalf"));
        matches.append(match2);
    } else if (matchesRunnerKeywords({m_power, m_sleep, m_suspend, m_toRam}, type, term)) {
        if (m_session->canSuspend()) {
            addSuspendMatch(SuspendState, matches, type);
        }
    } else if (matchesRunnerKeywords({m_power, m_sleep, m_suspend, m_hibernate, m_toDisk}, type, term)) {
        if (m_session->canHibernate()) {
            addSuspendMatch(HibernateState, matches, type);
        }
    } else if (matchesRunnerKeywords({m_power, m_sleep, m_suspend, m_hybrid, m_hybridSuspend}, type, term)) {
        if (m_session->canHybridSuspend()) {
            addSuspendMatch(HybridSuspendState, matches, type);
        }
    }

    context.addMatches(matches);
}

void PowerDevilRunner::addSuspendMatch(int value, QList<KRunner::QueryMatch> &matches, KRunner::QueryMatch::Type type)
{
    KRunner::QueryMatch match(this);
    match.setType(type);

    switch ((SleepState)value) {
    case SuspendState:
        match.setIconName(QStringLiteral("system-suspend"));
        match.setText(i18nc("Suspend to RAM", "Sleep"));
        match.setSubtext(i18n("Suspend to RAM"));
        match.setRelevance(1);
        break;
    case HibernateState:
        match.setIconName(QStringLiteral("system-suspend-hibernate"));
        match.setText(i18nc("Suspend to disk", "Hibernate"));
        match.setSubtext(i18n("Suspend to disk"));
        match.setRelevance(0.99);
        break;
    case HybridSuspendState:
        match.setIconName(QStringLiteral("system-suspend-hybrid"));
        match.setText(i18nc("Suspend to both RAM and disk", "Hybrid sleep"));
        match.setSubtext(i18n("Sleep now and fall back to hibernate"));
        match.setRelevance(0.98);
        break;
    }

    match.setData(value);
    match.setId(QStringLiteral("Sleep"));
    matches.append(match);
}

void PowerDevilRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    QDBusInterface iface(QStringLiteral("org.kde.Solid.PowerManagement"),
                         QStringLiteral("/org/kde/Solid/PowerManagement"),
                         QStringLiteral("org.kde.Solid.PowerManagement"));
    QDBusInterface brightnessIface(QStringLiteral("org.kde.Solid.PowerManagement"),
                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"));
    const QString action = match.id().remove(AbstractRunner::id() + QLatin1Char('_'));
    if (action == QLatin1String("BrightnessChange")) {
        QDBusReply<int> max = brightnessIface.call("brightnessMax");
        const int value = max.isValid() ? std::round(match.data().toInt() * max / 100.0) : match.data().toInt();
        brightnessIface.asyncCall("setBrightness", value);
    } else if (action == QLatin1String("DimTotal")) {
        brightnessIface.asyncCall(QStringLiteral("setBrightness"), 0);
    } else if (action == QLatin1String("DimHalf")) {
        QDBusReply<int> brightness = brightnessIface.asyncCall(QStringLiteral("brightness"));
        brightnessIface.asyncCall(QStringLiteral("setBrightness"), static_cast<int>(brightness / 2));
    } else if (action == QLatin1String("Sleep")) {
        switch ((SleepState)match.data().toInt()) {
        case SuspendState: {
            if (m_session->canSuspendThenHibernate()) {
                const QDBusReply<QString> currProfile = iface.call(QStringLiteral("currentProfile"));
                const KConfigGroup config(KConfig(QStringLiteral("powermanagementprofilesrc")).group(currProfile).group("SuspendSession"));
                if (config.readEntry<bool>("suspendThenHibernate", false)) {
                    m_session->suspendThenHibernate();
                    break;
                }
            }
            m_session->suspend();
            break;
        }
        case HibernateState:
            m_session->hibernate();
            break;
        case HybridSuspendState:
            m_session->hybridSuspend();
            break;
        }
    }
}

bool PowerDevilRunner::matchesRunnerKeywords(const QList<RunnerKeyword> &keywords, KRunner::QueryMatch::Type &type, const QString &query) const
{
    return std::any_of(keywords.begin(), keywords.end(), [&query, &type](const RunnerKeyword &keyword) {
        bool exactMatch =
            keyword.triggerWord.compare(query, Qt::CaseInsensitive) == 0 || keyword.translatedTriggerWord.compare(query, Qt::CaseInsensitive) == 0;
        type = exactMatch ? KRunner::QueryMatch::ExactMatch : KRunner::QueryMatch::CompletionMatch;
        if (!exactMatch && keyword.supportPartialMatch) {
            return keyword.triggerWord.startsWith(query, Qt::CaseInsensitive) || keyword.translatedTriggerWord.startsWith(query, Qt::CaseInsensitive);
        }
        return exactMatch;
    });
}

void PowerDevilRunner::addSyntaxForKeyword(const QList<RunnerKeyword> &keywords, const QString &description)
{
    QStringList exampleQueries;
    for (const auto &keyword : keywords) {
        exampleQueries << keyword.translatedTriggerWord;
    }
    addSyntax(exampleQueries, description);
}

int PowerDevilRunner::matchesScreenBrightnessKeywords(const QString &query) const
{
    const static QStringList expressions = {
        QStringLiteral("screen brightness "),
        i18nc("Note this is a KRunner keyword, it should end with a space", "screen brightness "),
        QStringLiteral("dim screen "),
        i18nc("Note this is a KRunner keyword, it should end with a space", "dim screen "),
    };

    for (const QString &expression : expressions) {
        if (query.startsWith(expression)) {
            const QString number = query.mid(expression.size());
            bool ok;
            int result = qBound(0, number.toInt(&ok), 100);
            return ok ? result : -1;
        }
    }
    return -1;
}

#include "PowerDevilRunner.moc"
