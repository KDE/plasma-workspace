/*
    SPDX-FileCopyrightText: 2008 Dario Freddi <drf@kdemod.ath.cx>
    SPDX-FileCopyrightText: 2008 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "PowerDevilRunner.h"

// kde-workspace/libs
#include <sessionmanagement.h>

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QIcon>

#include <KLocalizedString>
#include <KSharedConfig>

#include <cmath>

K_PLUGIN_CLASS_WITH_JSON(PowerDevilRunner, "plasma-runner-powerdevil.json")

PowerDevilRunner::PowerDevilRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
    , m_session(new SessionManagement(this))
{
    setObjectName(QStringLiteral("PowerDevil"));
    updateStatus();
    setMinLetterCount(3);
    const KLocalizedString suspend = ki18nc("Note this is a KRunner keyword", "suspend");
    m_suspend = RunnerKeyword{suspend.untranslatedText(), suspend.toString()};
    const KLocalizedString toRam = ki18nc("Note this is a KRunner keyword", "to ram");
    m_toRam = RunnerKeyword{toRam.untranslatedText(), toRam.toString(), false};
    const KLocalizedString sleep = ki18nc("Note this is a KRunner keyword", "sleep");
    m_sleep = RunnerKeyword{sleep.untranslatedText(), sleep.toString()};
    const KLocalizedString hibernate = ki18nc("Note this is a KRunner keyword", "hibernate");
    m_hibernate = RunnerKeyword{hibernate.untranslatedText(), hibernate.toString()};
    const KLocalizedString toDisk = ki18nc("Note this is a KRunner keyword", "to disk");
    m_toDisk = RunnerKeyword{toDisk.untranslatedText(), toDisk.toString(), false};
    const KLocalizedString dimScreen = ki18nc("Note this is a KRunner keyword", "dim screen");
    m_dimScreen = RunnerKeyword{dimScreen.untranslatedText(), dimScreen.toString()};
    const KLocalizedString screenBrightness = ki18nc("Note this is a KRunner keyword", "dim screen");
    m_screenBrightness = RunnerKeyword{screenBrightness.untranslatedText(), screenBrightness.toString()};
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

    Plasma::RunnerSyntax brightnessSyntax(i18nc("Note this is a KRunner keyword", "screen brightness"),
                                          // xgettext:no-c-format
                                          i18n("Lists screen brightness options or sets it to the brightness defined by :q:; "
                                               "e.g. screen brightness 50 would dim the screen to 50% maximum brightness"));
    brightnessSyntax.addExampleQuery(i18nc("Note this is a KRunner keyword", "dim screen"));
    addSyntax(brightnessSyntax);
}

PowerDevilRunner::~PowerDevilRunner()
{
}

void PowerDevilRunner::updateStatus()
{
    updateSyntaxes();
}

enum SleepState { StandbyState = 1, SuspendState = 2, HibernateState = 4, HybridSuspendState = 8 };

void PowerDevilRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    Plasma::QueryMatch::Type type = Plasma::QueryMatch::ExactMatch;
    QList<Plasma::QueryMatch> matches;

    int screenBrightnessResults = matchesScreenBrightnessKeywords(term);
    if (screenBrightnessResults != -1) {
        Plasma::QueryMatch match(this);
        match.setType(type);
        match.setIconName(QStringLiteral("preferences-system-power-management"));
        match.setText(i18n("Set Brightness to %1%", screenBrightnessResults));
        match.setData(screenBrightnessResults);
        match.setRelevance(1);
        match.setId(QStringLiteral("BrightnessChange"));
        matches.append(match);
    } else if (matchesRunnerKeywords({m_screenBrightness, m_dimScreen}, type, term)) {
        Plasma::QueryMatch match1(this);
        match1.setType(Plasma::QueryMatch::ExactMatch);
        match1.setIconName(QStringLiteral("preferences-system-power-management"));
        match1.setText(i18n("Dim screen totally"));
        match1.setRelevance(1);
        match1.setId(QStringLiteral("DimTotal"));
        matches.append(match1);

        Plasma::QueryMatch match2(this);
        match2.setType(type);
        match2.setIconName(QStringLiteral("preferences-system-power-management"));
        match2.setText(i18n("Dim screen by half"));
        match2.setRelevance(1);
        match2.setId(QStringLiteral("DimHalf"));
        matches.append(match2);
    } else if (matchesRunnerKeywords({m_sleep}, type, term)) {
        if (m_session->canSuspend()) {
            addSuspendMatch(SuspendState, matches, type);
        }

        if (m_session->canHibernate()) {
            addSuspendMatch(HibernateState, matches, type);
        }
    } else if (matchesRunnerKeywords({m_suspend, m_toRam}, type, term)) {
        addSuspendMatch(SuspendState, matches, type);
    } else if (matchesRunnerKeywords({m_hibernate, m_toDisk}, type, term)) {
        addSuspendMatch(HibernateState, matches, type);
    }

    context.addMatches(matches);
}

void PowerDevilRunner::addSuspendMatch(int value, QList<Plasma::QueryMatch> &matches, Plasma::QueryMatch::Type type)
{
    Plasma::QueryMatch match(this);
    match.setType(type);

    switch ((SleepState)value) {
    case SuspendState:
    case StandbyState:
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
    }

    match.setData(value);
    match.setId(QStringLiteral("Sleep"));
    matches.append(match);
}

void PowerDevilRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
{
    Q_UNUSED(context)

    QDBusInterface iface(QStringLiteral("org.kde.Solid.PowerManagement"),
                         QStringLiteral("/org/kde/Solid/PowerManagement"),
                         QStringLiteral("org.kde.Solid.PowerManagement"));
    QDBusInterface brightnessIface(QStringLiteral("org.kde.Solid.PowerManagement"),
                                   QStringLiteral("/org/kde/Solid/PowerManagement/Actions/BrightnessControl"),
                                   QStringLiteral("org.kde.Solid.PowerManagement.Actions.BrightnessControl"));
    if (match.id().startsWith(QLatin1String("PowerDevil_ProfileChange"))) {
        iface.asyncCall(QStringLiteral("loadProfile"), match.data().toString());
    } else if (match.id() == QLatin1String("PowerDevil_BrightnessChange")) {
        QDBusReply<int> max = brightnessIface.call("brightnessMax");
        const int value = max.isValid() ? std::round(match.data().toInt() * max / 100.0) : match.data().toInt();
        brightnessIface.asyncCall("setBrightness", value);
    } else if (match.id() == QLatin1String("PowerDevil_DimTotal")) {
        brightnessIface.asyncCall(QStringLiteral("setBrightness"), 0);
    } else if (match.id() == QLatin1String("PowerDevil_DimHalf")) {
        QDBusReply<int> brightness = brightnessIface.asyncCall(QStringLiteral("brightness"));
        brightnessIface.asyncCall(QStringLiteral("setBrightness"), static_cast<int>(brightness / 2));
    } else if (match.id().startsWith(QLatin1String("PowerDevil_Sleep"))) {
        switch ((SleepState)match.data().toInt()) {
        case SuspendState:
        case StandbyState:
            m_session->suspend();
            break;
        case HibernateState:
            m_session->hibernate();
            break;
        }
    }
}

bool PowerDevilRunner::matchesRunnerKeywords(const QList<RunnerKeyword> &keywords, Plasma::QueryMatch::Type &type, const QString &query) const
{
    return std::any_of(keywords.begin(), keywords.end(), [&query, &type](const RunnerKeyword &keyword) {
        bool exactMatch =
            keyword.triggerWord.compare(query, Qt::CaseInsensitive) == 0 || keyword.translatedTriggerWord.compare(query, Qt::CaseInsensitive) == 0;
        type = exactMatch ? Plasma::QueryMatch::ExactMatch : Plasma::QueryMatch::CompletionMatch;
        if (!exactMatch && keyword.supportPartialMatch) {
            return keyword.triggerWord.startsWith(query, Qt::CaseInsensitive) || keyword.translatedTriggerWord.startsWith(query, Qt::CaseInsensitive);
        }
        return exactMatch;
    });
}

void PowerDevilRunner::addSyntaxForKeyword(const QList<RunnerKeyword> &keywords, const QString &description)
{
    Plasma::RunnerSyntax syntax(keywords.first().translatedTriggerWord, description);
    for (int i = 1; i < keywords.size(); ++i) {
        syntax.addExampleQuery(keywords.at(i).translatedTriggerWord);
    }
    addSyntax(syntax);
}

int PowerDevilRunner::matchesScreenBrightnessKeywords(const QString &query) const
{
    const static QStringList expressions = {QStringLiteral("screen brightness "),
                                            i18nc("Note this is a KRunner keyword, it should end with a space", "screen brightness "),
                                            QStringLiteral("dim screen "),
                                            i18nc("Note this is a KRunner keyword, it should end with a space", "dim screen ")};

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
