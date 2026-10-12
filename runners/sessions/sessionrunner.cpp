/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2022 Natalie Clarius <natalie_clarius@yahoo.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sessionrunner.h"

#include <KLocalizedString>
#include <algorithm>

K_PLUGIN_CLASS_WITH_JSON(SessionRunner, "plasma-runner-sessions.json")

using namespace Qt::StringLiterals;

SessionRunner::SessionRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    setMinLetterCount(2);

    m_logoutKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to log out of the session", "logout;log out")
                           .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canLogout()) {
        addSyntax(m_logoutKeywords, i18n("Logs out, exiting the current desktop session"));
    }

    m_shutdownKeywords =
        i18nc("KRunner keywords (split by semicolons without whitespace) to shut down the computer", "shutdown;shut down;power;poweroff;power off")
            .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canShutdown()) {
        addSyntax(m_shutdownKeywords, i18n("Turns off the computer"));
    }

    m_restartKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to restart the computer", "restart;reboot")
                            .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canReboot()) {
        addSyntax(m_restartKeywords, i18n("Reboots the computer"));
    }

    m_lockKeywords =
        i18nc("KRunner keywords (split by semicolons without whitespace) to lock the screen", "lock;lock screen").split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canLock()) {
        addSyntax(m_lockKeywords, i18n("Locks the current sessions and starts the screen saver"));
    }

    m_saveKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to save the desktop session", "save;save session")
                         .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canSaveSession()) {
        addSyntax(m_saveKeywords, i18n("Saves the current session for session restoration"));
    }

    m_usersKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to switch user sessions", "switch user;new session")
                          .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canSwitchUser()) {
        addSyntax(m_usersKeywords, i18n("Switch to a session as a different user"));
    }
}

static qreal keywordMatchScore(const QStringList &keywords, const QString &term)
{
    qreal score = 0;
    for (const QString &keyword : keywords) {
        if (term.compare(keyword, Qt::CaseInsensitive) == 0) {
            return 1;
        }
        if (keyword.startsWith(term, Qt::CaseInsensitive)) {
            score = std::max(score, qreal(term.size()) / keyword.size());
        }
    }
    return score;
}

void SessionRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();
    QList<KRunner::QueryMatch> matches;

    const auto matchAction = [&](const QStringList &keywords, bool available, const QString &text, const QString &iconName, int action) {
        if (!available) {
            return;
        }
        const qreal score = keywordMatchScore(keywords, term);
        if (score <= 0) {
            return;
        }
        KRunner::QueryMatch match(this);
        if (score >= 1) {
            match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Highest);
            match.setRelevance(0.9);
        } else {
            match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Low);
            match.setRelevance(0.9 * score);
        }
        match.setText(text);
        match.setIconName(iconName);
        match.setData(action);
        matches << match;
    };

    matchAction(m_logoutKeywords, m_session.canLogout(), i18nc("log out command", "Log Out"), u"system-log-out"_s, LogoutAction);
    matchAction(m_shutdownKeywords, m_session.canShutdown(), i18nc("turn off computer command", "Shut Down"), u"system-shutdown"_s, ShutdownAction);
    matchAction(m_restartKeywords, m_session.canReboot(), i18nc("restart computer command", "Restart"), u"system-reboot"_s, RestartAction);
    matchAction(m_lockKeywords, m_session.canLock(), i18nc("lock screen command", "Lock"), u"system-lock-screen"_s, LockAction);
    matchAction(m_saveKeywords, m_session.canSaveSession(), i18n("Save Session"), u"system-save-session"_s, SaveAction);
    matchAction(m_usersKeywords, m_session.canSwitchUser(), i18n("Switch User"), u"system-switch-user"_s, SwitchAction);

    context.addMatches(matches);
}

void SessionRunner::run(const KRunner::RunnerContext & /*context*/, const KRunner::QueryMatch &match)
{
    if (match.data().typeId() == QMetaType::Int) {
        switch (match.data().toInt()) {
        case LogoutAction:
            m_session.requestLogout();
            break;
        case RestartAction:
            m_session.requestReboot();
            break;
        case ShutdownAction:
            m_session.requestShutdown();
            break;
        case LockAction:
            m_session.lock();
            break;
        case SaveAction:
            m_session.saveSession();
            break;
        case SwitchAction:
            m_session.switchUser();
            break;
        }
        return;
    }
}

#include "sessionrunner.moc"

#include "moc_sessionrunner.cpp"
