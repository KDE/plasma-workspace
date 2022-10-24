/*
    SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2022 Natalie Clarius <natalie_clarius@yahoo.de>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "sessionrunner.h"

#include <KLocalizedString>
#include <QDebug>
#include <QMessageBox>

#include "kworkspace.h"

#include "screensaver_interface.h"

K_PLUGIN_CLASS_WITH_JSON(SessionRunner, "plasma-runner-sessions.json")

SessionRunner::SessionRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : Plasma::AbstractRunner(parent, metaData, args)
{
    setObjectName(QStringLiteral("Sessions"));
    setPriority(LowPriority);

    if (m_session.canLogout()) {
        Plasma::RunnerSyntax logoutSyntax(i18nc("log out command", "logout"), i18n("Logs out, exiting the current desktop session"));
        logoutSyntax.addExampleQuery(i18nc("log out command", "log out"));
        addSyntax(logoutSyntax);
    }

    if (m_session.canShutdown()) {
        Plasma::RunnerSyntax shutdownSyntax(i18nc("shut down computer command", "shutdown"), i18n("Turns off the computer"));
        shutdownSyntax.addExampleQuery(i18nc("shut down computer command", "shut down"));
        addSyntax(shutdownSyntax);
    }

    if (m_session.canReboot()) {
        Plasma::RunnerSyntax restartSyntax(i18nc("restart computer command", "restart"), i18n("Reboots the computer"));
        restartSyntax.addExampleQuery(i18nc("restart computer command", "reboot"));
        addSyntax(restartSyntax);
    }

    if (m_session.canLock()) {
        Plasma::RunnerSyntax lockSyntax(i18nc("lock screen command", "lock"), i18n("Locks the current sessions and starts the screen saver"));
        lockSyntax.addExampleQuery(i18nc("lock screen command", "lock screen"));
        addSyntax(lockSyntax);
    }

    if (m_session.canSaveSession()) {
        Plasma::RunnerSyntax saveSyntax(i18nc("save session command", "save"), i18n("Saves the current session for session restoration"));
        saveSyntax.addExampleQuery(i18nc("save session command", "save session"));
        addSyntax(saveSyntax);
    }

    if (m_session.canSwitchUser()) {
        Plasma::RunnerSyntax fastUserSwitchSyntax(i18nc("switch user command", "switch user"), i18n("Starts a new session as a different user"));
        fastUserSwitchSyntax.addExampleQuery(i18nc("new session command", "new session"));
    addSyntax(fastUserSwitchSyntax);
    }

    Plasma::RunnerSyntax listAllSyntax(QStringLiteral("SESSIONS"), i18n("Lists all sessions")); //"SESSIONS" should not be translated; it's used programmaticaly
    listAllSyntax.addExampleQuery(i18nc("list user sessions command", "sessions"));
    listAllSyntax.addExampleQuery(i18nc("list user sessions command", "session"));
    listAllSyntax.addExampleQuery(i18nc("list user sessions command", "desktop sessions"));
    listAllSyntax.addExampleQuery(i18nc("list user sessions command", "desktop session"));
    addSyntax(listAllSyntax);

    m_triggerWord = i18nc("switch user command", "switch");
    Plasma::RunnerSyntax triggerWordSyntax(m_triggerWord + QStringLiteral(" :q:"),
                                           i18n("Switches to the active session for the user :q:, or lists all active sessions if :q: is not provided"));
    addSyntax(triggerWordSyntax);

    setMinLetterCount(3);
}

SessionRunner::~SessionRunner()
{
}

void SessionRunner::matchCommands(QList<Plasma::QueryMatch> &matches, const QString &term)
{
    if (term.compare(i18nc("log out command", "logout"), Qt::CaseInsensitive) == 0
        || term.compare(i18nc("log out command", "log out"), Qt::CaseInsensitive) == 0) {
        if (m_session.canLogout()) {
            Plasma::QueryMatch match(this);
            match.setText(i18nc("log out command", "Logout"));
            match.setIconName(QStringLiteral("system-log-out"));
            match.setData(LogoutAction);
            match.setType(Plasma::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (term.compare(i18nc("shut down computer command", "shutdown"), Qt::CaseInsensitive) == 0
               || term.compare(i18nc("shut down computer command", "shut down"), Qt::CaseInsensitive) == 0) {
        if (m_session.canShutdown()) {
            Plasma::QueryMatch match(this);
            match.setText(i18n("Shut down the computer"));
            match.setIconName(QStringLiteral("system-shutdown"));
            match.setData(ShutdownAction);
            match.setType(Plasma::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (term.compare(i18nc("restart computer command", "restart"), Qt::CaseInsensitive) == 0
               || term.compare(i18nc("restart computer command", "reboot"), Qt::CaseInsensitive) == 0) {
        if (m_session.canReboot()) {
            Plasma::QueryMatch match(this);
            match.setText(i18n("Restart the computer"));
            match.setIconName(QStringLiteral("system-reboot"));
            match.setData(RestartAction);
            match.setType(Plasma::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (term.compare(i18nc("lock screen command", "lock"), Qt::CaseInsensitive) == 0 || term.compare(i18nc("lock screen command", "lock screen"), Qt::CaseInsensitive) == 0) {
        if (m_session.canLock()) {
            Plasma::QueryMatch match(this);
            match.setText(i18n("Restart the computer"));
            match.setIconName(QStringLiteral("system-reboot"));
            match.setData(RestartAction);
            match.setType(Plasma::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    } else if (term.compare(i18nc("save session command", "save"), Qt::CaseInsensitive) == 0
               || term.compare(i18nc("save session command", "save session"), Qt::CaseInsensitive) == 0) {
        if (m_session.canSaveSession()) {
            Plasma::QueryMatch match(this);
            match.setText(i18n("Save the session"));
            match.setSubtext(i18n("Save the current session for session restoration"));
            match.setIconName(QStringLiteral("system-save-session"));
            match.setData(SaveAction);
            match.setType(Plasma::QueryMatch::ExactMatch);
            match.setRelevance(0.9);
            matches << match;
        }
    }
}

void SessionRunner::match(Plasma::RunnerContext &context)
{
    const QString term = context.query();
    QString user;
    bool matchUser = false;

    QList<Plasma::QueryMatch> matches;

    // first compare with SESSIONS. this must *NOT* be translated (i18n)
    // as it is used as an internal command trigger (e.g. via d-bus),
    // not as a user supplied query. and yes, "Ugh, magic strings"
    bool listAll = term.compare(QLatin1String("SESSIONS"), Qt::CaseInsensitive) == 0
        || term.compare(i18nc("list user sessions command", "sessions"), Qt::CaseInsensitive) == 0
        || term.compare(i18nc("list user sessions command", "session"), Qt::CaseInsensitive) == 0
        || term.compare(i18nc("list user sessions command", "desktop sessions"), Qt::CaseInsensitive) == 0
        || term.compare(i18nc("list user sessions command", "desktop session"), Qt::CaseInsensitive) == 0;

    if (!listAll) {
        // no luck, try the "switch" user command
        if (term.startsWith(m_triggerWord, Qt::CaseInsensitive)) {
            user = term.right(term.size() - m_triggerWord.length()).trimmed();
            listAll = user.isEmpty();
            matchUser = !listAll;
        } else {
            // we know it's not SESSION or "switch <something>", so let's
            // try some other possibilities
            matchCommands(matches, term);
        }
    }

    bool switchUser = listAll || term.compare(i18nc("switch user comand", "switch user"), Qt::CaseInsensitive) == 0
        || term.compare(i18nc("switch user comand", "new session"), Qt::CaseInsensitive) == 0;

    if (switchUser && m_session.canSwitchUser() && dm.isSwitchable() && dm.numReserve() >= 0) {
        Plasma::QueryMatch match(this);
        match.setType(Plasma::QueryMatch::ExactMatch);
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
            Plasma::QueryMatch::Type type = Plasma::QueryMatch::NoMatch;
            qreal relevance = 0.7;

            if (listAll) {
                type = Plasma::QueryMatch::ExactMatch;
                relevance = 1;
            } else if (matchUser) {
                if (name.compare(user, Qt::CaseInsensitive) == 0) {
                    // we need an elif branch here because we don't
                    // want the last conditional to be checked if !listAll
                    type = Plasma::QueryMatch::ExactMatch;
                    relevance = 1;
                } else if (name.contains(user, Qt::CaseInsensitive)) {
                    type = Plasma::QueryMatch::PossibleMatch;
                }
            }

            if (type != Plasma::QueryMatch::NoMatch) {
                Plasma::QueryMatch match(this);
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

void SessionRunner::run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &match)
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

    // TODO: this message is too verbose and too technical.
    int result = QMessageBox::warning(nullptr,
                                      i18n("Warning - New Session"),
                                      i18n("<p>You have chosen to open another desktop session.<br />"
                                           "The current session will be hidden "
                                           "and a new login screen will be displayed.<br />"
                                           "An F-key is assigned to each session; "
                                           "F%1 is usually assigned to the first session, "
                                           "F%2 to the second session and so on. "
                                           "You can switch between sessions by pressing "
                                           "Ctrl, Alt and the appropriate F-key at the same time. "
                                           "Additionally, the Plasma Panel and Desktop menus have "
                                           "actions for switching between sessions.</p>",
                                           7,
                                           8));

    if (result == QMessageBox::Cancel) {
        return;
    }
    m_session.lock();

    dm.startReserve();
}

#include "sessionrunner.moc"
