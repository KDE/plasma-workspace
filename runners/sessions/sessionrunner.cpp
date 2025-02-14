/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2022 Natalie Clarius <natalie_clarius@yahoo.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sessionrunner.h"

#include <QApplication>

#include <KAboutData>
#include <KCrash>
#include <KGuiItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <KSharedConfig>

#include "config-workspace.h"
#include "dbusutils_p.h"

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication app(argc, argv); // KRun needs widgets for error message boxes

    KAboutData about(QStringLiteral("sessionrunner"), QString(), QStringLiteral(WORKSPACE_VERSION_STRING));
    KAboutData::setApplicationData(about);

    KCrash::initialize();

    SessionRunner r;
    return app.exec();
}

SessionRunner::SessionRunner(QObject *parent)
    : QObject(parent)
{
    m_logoutKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to log out of the session", "logout;log out")
                           .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canLogout()) {
        // TODO addSyntax(m_logoutKeywords, i18n("Logs out, exiting the current desktop session"));
    }

    m_shutdownKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to shut down the computer", "shutdown;shut down;power;power off")
                             .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canShutdown()) {
        // TODO addSyntax(m_shutdownKeywords, i18n("Turns off the computer"));
    }

    m_restartKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to restart the computer", "restart;reboot")
                            .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canReboot()) {
        // TODO addSyntax(m_restartKeywords, i18n("Reboots the computer"));
    }

    m_lockKeywords =
        i18nc("KRunner keywords (split by semicolons without whitespace) to lock the screen", "lock;lock screen").split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canLock()) {
        // TODO addSyntax(m_lockKeywords, i18n("Locks the current sessions and starts the screen saver"));
    }

    m_saveKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to save the desktop session", "save;save session")
                         .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canSaveSession()) {
        // TODO addSyntax(m_saveKeywords, i18n("Saves the current session for session restoration"));
    }

    m_usersKeywords = i18nc("KRunner keywords (split by semicolons without whitespace) to switch user sessions", "switch user;new session")
                          .split(QLatin1Char(';'), Qt::SkipEmptyParts);
    if (m_session.canSwitchUser()) {
        // TODO addSyntax(m_usersKeywords, i18n("Starts a new session as a different user"));
    }

    m_sessionsKeyword = i18nc("KRunner keyword to list user sessions", "sessions");
    // TODO addSyntax(m_sessionsKeyword, i18n("Lists all sessions"));

    m_switchKeyword = i18nc("KRunner keyword to switch user sessions", "switch");
    // TODO addSyntax(m_switchKeyword + QStringLiteral(" :q:"),
    //           i18n("Switches to the active session for the user :q:, or lists all active sessions if :q: is not provided"));

    // setMinLetterCount(3);
}

static inline bool anyKeywordMatches(const QStringList &keywords, const QString &term)
{
    return std::any_of(keywords.cbegin(), keywords.cend(), [&term](const QString &keyword) {
        return term.compare(keyword, Qt::CaseInsensitive) == 0;
    });
}

RemoteMatches SessionRunner::matchCommands(const QString &term)
{
    RemoteMatches matches;
    RemoteMatch match;
    match.categoryRelevance = qToUnderlying(KRunner::QueryMatch::CategoryRelevance::Highest);
    match.relevance = 0.9;
    if (anyKeywordMatches(m_logoutKeywords, term)) {
        if (m_session.canLogout()) {
            match.text = i18nc("log out command", "Log Out");
            match.iconName = QStringLiteral("system-log-out");
            match.properties[QStringLiteral("action")] = LogoutAction;
            matches << match;
        }
    } else if (anyKeywordMatches(m_shutdownKeywords, term)) {
        if (m_session.canShutdown()) {
            RemoteMatch match;
            match.text = i18nc("turn off computer command", "Shut Down");
            match.iconName = QStringLiteral("system-shutdown");
            match.properties[QStringLiteral("action")] = ShutdownAction;
            matches << match;
        }
    } else if (anyKeywordMatches(m_restartKeywords, term)) {
        if (m_session.canReboot()) {
            match.text = i18nc("restart computer command", "Restart");
            match.iconName = QStringLiteral("system-reboot");
            match.properties[QStringLiteral("action")] = RestartAction;
            matches << match;
        }
    } else if (anyKeywordMatches(m_lockKeywords, term)) {
        if (m_session.canLock()) {
            match.text = i18nc("lock screen command", "Lock");
            match.iconName = QStringLiteral("system-lock-screen");
            match.properties[QStringLiteral("action")] = LockAction;
            matches << match;
        }
    } else if (anyKeywordMatches(m_saveKeywords, term)) {
        if (m_session.canSaveSession()) {
            match.text = i18n("Save Session");
            match.iconName = QStringLiteral("system-save-session");
            match.properties[QStringLiteral("action")] = SaveAction;
            matches << match;
        }
    }
    return matches;
}

RemoteMatches SessionRunner::Match(const QString &term)
{
    QString user;
    bool matchUser = false;

    RemoteMatches matches;

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
            matches << matchCommands(term);
        }
    }

    bool switchUser = listAll || anyKeywordMatches(m_usersKeywords, term);

    if (switchUser && m_session.canSwitchUser() && dm.isSwitchable() && dm.numReserve() >= 0) {
        RemoteMatch match;
        match.categoryRelevance = qToUnderlying(KRunner::QueryMatch::CategoryRelevance::Highest);
        match.iconName = QStringLiteral("system-switch-user");
        match.text = i18n("Switch User");
        matches << match;
    }

    // now add the active sessions
    if (listAll || matchUser) {
        SessList sessions;
        dm.localSessions(sessions);

        for (const SessEnt &session : std::as_const(sessions)) {
            if (!session.vt || session.self) {
                continue;
            }

            QString name = KDisplayManager::sess2Str(session);
            KRunner::QueryMatch::CategoryRelevance categoryRelevance;
            qreal relevance = 0.7;

            if (listAll) {
                categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Highest;
                relevance = 1;
            } else if (matchUser) {
                const int nameIdx = name.indexOf(user, Qt::CaseInsensitive);
                if (nameIdx == 0 && name.size() == user.size()) {
                    // we need an elif branch here because we don't
                    // want the last conditional to be checked if !listAll
                    categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Highest;
                    relevance = 1;
                } else if (nameIdx == 0) {
                    categoryRelevance = KRunner::QueryMatch::CategoryRelevance::Moderate;
                } else {
                    continue;
                }
            } else {
                continue;
            }

            RemoteMatch match;
            match.categoryRelevance = qToUnderlying(categoryRelevance);
            match.relevance = relevance;
            match.iconName = QStringLiteral("user-identity");
            match.text = name;
            match.properties[QStringLiteral("terminal")] = QString::number(session.vt);
            matches << match;
        }
    }

    return matches;
}

void SessionRunner::Run(const QString &id, const QString &actionId)
{
    /*
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
    continueButton.setText(u"Enter new session"_s);
    KGuiItem cancelButton = KStandardGuiItem::cancel();
    cancelButton.setText(u"Stay in current session"_s);
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
    }*/
}

#include "sessionrunner.moc"

#include "moc_sessionrunner.cpp"
