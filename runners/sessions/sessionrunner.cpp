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

SessionRunner::SessionRunner(QObject *parent, const KPluginMetaData &metaData)
    : KRunner::AbstractRunner(parent, metaData)
{
    setObjectName(QStringLiteral("Sessions"));
    setPriority(LowPriority);

    m_logoutKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to log out of the session", "logout;log out")
                           .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canLogout()) {
        KRunner::RunnerSyntax logoutSyntax(m_logoutKeywords.first(), i18n("Logs out, exiting the current desktop session"));
        for (QString keyword : m_logoutKeywords.mid(1)) {
            logoutSyntax.addExampleQuery(keyword);
        }
        addSyntax(logoutSyntax);
    }

    m_shutdownKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to shut down the computer", "shutdown;shut down")
                             .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canShutdown()) {
        KRunner::RunnerSyntax shutdownSyntax(m_shutdownKeywords.first(), i18n("Turns off the computer"));
        for (QString keyword : m_shutdownKeywords.mid(1)) {
            shutdownSyntax.addExampleQuery(keyword);
        }
        addSyntax(shutdownSyntax);
    }

    m_restartKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to restart the computer", "restart;reboot")
                            .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canReboot()) {
        KRunner::RunnerSyntax restartSyntax(m_restartKeywords.first(), i18n("Reboots the computer"));
        for (QString keyword : m_restartKeywords.mid(1)) {
            restartSyntax.addExampleQuery(keyword);
        }
        addSyntax(restartSyntax);
    }

    m_lockKeywords =
        i18nc("KRunner keywords (split by semicolons without whitespace) to lock the screen", "lock;lock screen").split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canLock()) {
        KRunner::RunnerSyntax lockSyntax(m_lockKeywords.first(), i18n("Locks the current sessions and starts the screen saver"));
        for (QString keyword : m_lockKeywords.mid(1)) {
            lockSyntax.addExampleQuery(keyword);
        }
        addSyntax(lockSyntax);
    }

    m_saveKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to save the desktop session", "save;save session")
                         .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canSaveSession()) {
        KRunner::RunnerSyntax saveSyntax(m_saveKeywords.first(), i18n("Saves the current session for session restoration"));
        for (QString keyword : m_saveKeywords.mid(1)) {
            saveSyntax.addExampleQuery(keyword);
        }
        addSyntax(saveSyntax);
    }

    m_usersKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to switch user sessions", "switch user;new session")
                          .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canSwitchUser()) {
        KRunner::RunnerSyntax usersSyntax(m_usersKeywords.first(), i18n("Starts a new session as a different user"));
        for (QString keyword : m_usersKeywords.mid(1)) {
            usersSyntax.addExampleQuery(keyword);
        }
        addSyntax(usersSyntax);
    }

    m_sessionsKeyword = i18nc("KRunner keyword to list user sessions", "sessions");
    KRunner::RunnerSyntax sessionsSyntax(m_sessionsKeyword, i18n("Lists all sessions"));
    addSyntax(sessionsSyntax);

    m_switchKeyword = i18nc("KRunner keyword to switch user sessions", "switch");
    KRunner::RunnerSyntax switchSyntax(m_switchKeyword + QStringLiteral(" :q:"),
                                       i18n("Switches to the active session for the user :q:, or lists all active sessions if :q: is not provided"));
    addSyntax(switchSyntax);

    setMinLetterCount(3);
}

SessionRunner::~SessionRunner()
{
}

static inline bool anyKeywordMatches(const QStringList &keywords, const QString &term)
{
    return std::any_of(keywords.cbegin(), keywords.cend(), [&term](const QString &keyword) {
        return term.compare(keyword, Qt::CaseInsensitive) == 0;
    });
}

void SessionRunner::matchCommands(QList<KRunner::QueryMatch> &matches, const QString &term)
{
    if (anyKeywordMatches(m_logoutKeywords, term)) {
        if (m_session.canLogout()) {
            KRunner::QueryMatch match(this);
            match.setText(i18nc("log out command", "Log Out"));
            match.setIconName(QStringLiteral("system-log-out"));
            match.setData(LogoutAction);
            match.setType(KRunner::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (anyKeywordMatches(m_shutdownKeywords, term)) {
        if (m_session.canShutdown()) {
            KRunner::QueryMatch match(this);
            match.setText(i18nc("turn off computer command", "Shut Down"));
            match.setIconName(QStringLiteral("system-shutdown"));
            match.setData(ShutdownAction);
            match.setType(KRunner::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (anyKeywordMatches(m_restartKeywords, term)) {
        if (m_session.canReboot()) {
            KRunner::QueryMatch match(this);
            match.setText(i18nc("restart computer command", "Restart"));
            match.setIconName(QStringLiteral("system-reboot"));
            match.setData(RestartAction);
            match.setType(KRunner::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (anyKeywordMatches(m_lockKeywords, term)) {
        if (m_session.canLock()) {
            KRunner::QueryMatch match(this);
            match.setText(i18nc("lock screen command", "Lock"));
            match.setIconName(QStringLiteral("system-lock-screen"));
            match.setData(LockAction);
            match.setType(KRunner::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (anyKeywordMatches(m_saveKeywords, term)) {
        if (m_session.canSaveSession()) {
            KRunner::QueryMatch match(this);
            match.setText(i18n("Save Session"));
            match.setIconName(QStringLiteral("system-save-session"));
            match.setData(SaveAction);
            match.setType(KRunner::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    }
}

void SessionRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();
    QString user;
    bool matchUser = false;

    QList<KRunner::QueryMatch> matches;

    // first compare with "sessions" keyword
    // "SESSIONS" must *NOT* be translated (i18n)
    // as it is used as an internal command trigger (e.g. via d-bus),
    // not as a user supplied query. and yes, "Ugh, magic strings"
    bool listAll = anyKeywordMatches(QStringList({QStringLiteral("SESSIONS"), m_sessionsKeyword}), term);

    if (!listAll) {
        // no luck, try the "switch" user command
        if (term.startsWith(m_switchKeyword, Qt::CaseInsensitive)) {
            user = term.right(term.size() - m_switchKeyword.length()).trimmed();
            listAll = user.isEmpty();
            matchUser = !listAll;
        } else {
            // we know it's not "sessions" or "switch <something>", so let's
            // try some other possibilities
            matchCommands(matches, term);
        }
    }

    bool switchUser = listAll || anyKeywordMatches(m_usersKeywords, term);

    if (switchUser && m_session.canSwitchUser() && dm.isSwitchable() && dm.numReserve() >= 0) {
        KRunner::QueryMatch match(this);
        match.setType(KRunner::QueryMatch::ExactMatch);
        match.setIconName(QStringLiteral("system-switch-user"));
        match.setText(i18n("Switch User"));
        matches << match;
    }

    // now add the active sessions
    if (listAll || matchUser) {
        SessList sessions;
        dm.localSessions(sessions);

        for (const SessEnt &session : qAsConst(sessions)) {
            if (!session.vt || session.self) {
                continue;
            }

            QString name = KDisplayManager::sess2Str(session);
            KRunner::QueryMatch::Type type = KRunner::QueryMatch::NoMatch;
            qreal relevance = 0.7;

            if (listAll) {
                type = KRunner::QueryMatch::ExactMatch;
                relevance = 1;
            } else if (matchUser) {
                if (name.compare(user, Qt::CaseInsensitive) == 0) {
                    // we need an elif branch here because we don't
                    // want the last conditional to be checked if !listAll
                    type = KRunner::QueryMatch::ExactMatch;
                    relevance = 1;
                } else if (name.contains(user, Qt::CaseInsensitive)) {
                    type = KRunner::QueryMatch::PossibleMatch;
                }
            }

            if (type != KRunner::QueryMatch::NoMatch) {
                KRunner::QueryMatch match(this);
                match.setType(type);
                match.setRelevance(relevance);
                match.setIconName(QStringLiteral("user-identity"));
                match.setText(name);
                match.setData(QString::number(session.vt));
                matches << match;
            }
        }
    }

    context.addMatches(matches);
}

void SessionRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    Q_UNUSED(context);
    if (match.data().type() == QVariant::Int) {
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
        }
        return;
    }

    if (!match.data().toString().isEmpty()) {
        dm.lockSwitchVT(match.data().toString().toInt());
        return;
    }

    const auto config = KSharedConfig::openConfig(QStringLiteral("ksmserverrc"));
    KMessageBox::setDontShowAgainConfig(config.data());
    KGuiItem continueButton = KStandardGuiItem::cont();
    continueButton.setText("Enter new session");
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    cancelButton.setText("Stay in current session");
    KMessageBox::ButtonCode confirmNewSession =
        KMessageBox::warningContinueCancel(nullptr,
                                           i18n("<p>You are about to enter a new desktop session.</p>"
                                                "<p>A login screen will be displayed and the current session will be hidden.</p>"
                                                "<p>You can switch between desktop sessions using:</p>"
                                                "<ul>"
                                                "<li>Ctrl+Alt+F{number of session}</li>"
                                                "<li>Plasma search (type '%1')</li>"
                                                "<li>Plasma widgets (such as the application launcher)</li>"
                                                "</ul>",
                                                m_switchKeyword),
                                           i18n("New Desktop Session"),
                                           continueButton,
                                           cancelButton,
                                           QStringLiteral("ConfirmNewSession"));
    if (confirmNewSession == KMessageBox::Continue) {
        m_session.lock();
        dm.startReserve();
    }
}

#include "sessionrunner.moc"
