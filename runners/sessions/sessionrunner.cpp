/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2022 Natalie Clarius <natalie_clarius@yahoo.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sessionrunner.h"

#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>

K_PLUGIN_CLASS_WITH_JSON(SessionRunner, "plasma-runner-sessions.json")

using namespace Qt::StringLiterals;

SessionRunner::SessionRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    m_logoutKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to log out of the session", "logout;log out")
                           .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canLogout()) {
        addSyntax(m_logoutKeywords, i18n("Logs out, exiting the current desktop session"));
    }

    m_shutdownKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to shut down the computer", "shutdown;shut down;power;power off")
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
    setMinLetterCount(3);
}

static inline bool anyKeywordMatches(const QStringList &keywords, const QString &term)
{
    return std::any_of(keywords.cbegin(), keywords.cend(), [&term](const QString &keyword) {
        return term.compare(keyword, Qt::CaseInsensitive) == 0;
    });
}

void SessionRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();
    QList<KRunner::QueryMatch> matches;

    KRunner::QueryMatch match(this);
    match.setCategoryRelevance(KRunner::QueryMatch::CategoryRelevance::Highest);
    match.setRelevance(0.9);
    if (anyKeywordMatches(m_logoutKeywords, term)) {
        if (m_session.canLogout()) {
            match.setText(i18nc("log out command", "Log Out"));
            match.setIconName(QStringLiteral("system-log-out"));
            match.setData(LogoutAction);
            matches << match;
        }
    } else if (anyKeywordMatches(m_shutdownKeywords, term)) {
        if (m_session.canShutdown()) {
            KRunner::QueryMatch match(this);
            match.setText(i18nc("turn off computer command", "Shut Down"));
            match.setIconName(QStringLiteral("system-shutdown"));
            match.setData(ShutdownAction);
            matches << match;
        }
    } else if (anyKeywordMatches(m_restartKeywords, term)) {
        if (m_session.canReboot()) {
            match.setText(i18nc("restart computer command", "Restart"));
            match.setIconName(QStringLiteral("system-reboot"));
            match.setData(RestartAction);
            matches << match;
        }
    } else if (anyKeywordMatches(m_lockKeywords, term)) {
        if (m_session.canLock()) {
            match.setText(i18nc("lock screen command", "Lock"));
            match.setIconName(QStringLiteral("system-lock-screen"));
            match.setData(LockAction);
            matches << match;
        }
    } else if (anyKeywordMatches(m_saveKeywords, term)) {
        if (m_session.canSaveSession()) {
            match.setText(i18n("Save Session"));
            match.setIconName(QStringLiteral("system-save-session"));
            match.setData(SaveAction);
            matches << match;
        }
    } else if (anyKeywordMatches(m_usersKeywords, term)) {
        if (m_session.canSwitchUser()) {
            match.setText(i18n("Switch User"));
            match.setIconName(QStringLiteral("system-switch-user"));
            match.setData(SwitchAction);
            matches << match;
        }
    }
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
