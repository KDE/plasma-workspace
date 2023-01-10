/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>

    SPDX-FileContributor: Oswald Buddenhagen <ob6@inf.tu-dresden.de>

    some code taken from the dcopserver (part of the KDE libraries), which is
    SPDX-FileCopyrightText: 1999 Matthias Ettrich <ettrich@kde.org>
    SPDX-FileCopyrightText: 1999 Preston Brown <pbrown@kde.org>

    SPDX-License-Identifier: MIT
*/

#include <config-ksmserver.h>
#include <config-unix.h> // HAVE_LIMITS_H
#include <config-workspace.h>

#include <ksmserver_debug.h>

#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <QApplication>
#include <QFile>
#include <QFutureWatcher>
#include <QTimer>
#include <QtConcurrentRun>

#include "client.h"
#include "global.h"
#include "server.h"
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KNotification>
#include <KSharedConfig>
#include <kdisplaymanager.h>

#include "kwinsession_interface.h"
#include "logoutprompt_interface.h"
#include "shutdown_interface.h"

enum KWinSessionState {
    Normal = 0,
    Saving = 1,
    Quitting = 2,
};

void KSMServer::logout(int confirm, int sdtype, int sdmode)
{
    // KDE5: remove me
    if (sdtype == KWorkSpace::ShutdownTypeLogout)
        sdtype = KWorkSpace::ShutdownTypeNone;

    shutdown((KWorkSpace::ShutdownConfirm)confirm, (KWorkSpace::ShutdownType)sdtype, (KWorkSpace::ShutdownMode)sdmode);
}

bool KSMServer::closeSession()
{
    qCDebug(KSMSERVER) << "Close session called. Current state is:" << state;

    Q_ASSERT(calledFromDBus());
    setDelayedReply(true);

    const QDBusMessage callerContext = message();

    auto conn = QSharedPointer<QMetaObject::Connection>::create(QMetaObject::Connection());
    *conn = connect(this, &KSMServer::logoutFinished, this, [callerContext, conn](bool sessionClosed) {
        auto reply = callerContext.createReply(sessionClosed);
        QDBusConnection::sessionBus().send(reply);
        QObject::disconnect(*conn);
    });

    performLogout();
    return false;
}

bool KSMServer::canShutdown()
{
    return KDisplayManager().canShutdown();
}

bool KSMServer::isShuttingDown() const
{
    return state >= Shutdown;
}

// this method exists purely for compatibility
void KSMServer::shutdown(KWorkSpace::ShutdownConfirm confirm, KWorkSpace::ShutdownType sdtype, KWorkSpace::ShutdownMode sdmode)
{
    qCDebug(KSMSERVER) << "Shutdown called with confirm " << confirm << " type " << sdtype << " and mode " << sdmode;
    if (state >= Shutdown) // already performing shutdown
        return;
    if (state != Idle) // performing startup
    {
        return;
    }

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration(); // config may have changed in the KControl module

    KConfigGroup cg(config, "General");

    bool logoutConfirmed = (confirm == KWorkSpace::ShutdownConfirmYes      ? false
                                : confirm == KWorkSpace::ShutdownConfirmNo ? true
                                                                           : !cg.readEntry("confirmLogout", true));

    int shutdownType = (sdtype != KWorkSpace::ShutdownTypeDefault ? sdtype : cg.readEntry("shutdownType", (int)KWorkSpace::ShutdownType::ShutdownTypeLogout));

    if (!logoutConfirmed) {
        OrgKdeLogoutPromptInterface logoutPrompt(QStringLiteral("org.kde.LogoutPrompt"), QStringLiteral("/LogoutPrompt"), QDBusConnection::sessionBus());
        switch (shutdownType) {
        case KWorkSpace::ShutdownTypeHalt:
            logoutPrompt.promptShutDown();
            break;
        case KWorkSpace::ShutdownTypeReboot:
            logoutPrompt.promptReboot();
            break;
        case KWorkSpace::ShutdownTypeNone:
            Q_FALLTHROUGH();
        default:
            logoutPrompt.promptLogout();
            break;
        }
    } else {
        OrgKdeShutdownInterface shutdownIface(QStringLiteral("org.kde.Shutdown"), QStringLiteral("/Shutdown"), QDBusConnection::sessionBus());
        switch (shutdownType) {
        case KWorkSpace::ShutdownTypeHalt:
            shutdownIface.logoutAndShutdown();
            break;
        case KWorkSpace::ShutdownTypeReboot:
            shutdownIface.logoutAndReboot();
            break;
        case KWorkSpace::ShutdownTypeNone:
            Q_FALLTHROUGH();
        default:
            shutdownIface.logout();
            break;
        }
    }
}

void KSMServer::performLogout()
{
    if (state >= Shutdown) { // already performing shutdown
        return;
    }
    if (state != Idle) {
        QTimer::singleShot(1000, this, &KSMServer::performLogout);
    }

    auto setStateReply = m_kwinInterface->setState(KWinSessionState::Saving);

    state = Shutdown;

    // shall we save the session on logout?
    KConfigGroup cg(KSharedConfig::openConfig(), "General");
    saveSession = (cg.readEntry("loginMode", QStringLiteral("restorePreviousLogout")) == QLatin1String("restorePreviousLogout"));

    qCDebug(KSMSERVER) << "saveSession is " << saveSession;

    if (saveSession)
        sessionGroup = QStringLiteral("Session: ") + QString::fromLocal8Bit(SESSION_PREVIOUS_LOGOUT);

    saveType = saveSession ? SmSaveBoth : SmSaveGlobal;
#ifndef NO_LEGACY_SESSION_MANAGEMENT
    performLegacySessionSave();
#endif
    startProtection();

    // Tell KWin to start saving before we start tearing down clients
    // as any "Save changes?" prompt might meddle with the state
    if (saveSession) {
        setStateReply.waitForFinished(); // do we have to wait for this to finish?

        qCDebug(KSMSERVER) << "Telling KWin we're about to save session" << currentSession();

        auto saveSessionCall = m_kwinInterface->aboutToSaveSession(currentSession());
        // We need to wait for KWin to save the initial state, e.g. active client and
        // current desktop before we signal any clients to quit. They might bring up
        // "Save changes?" prompts altering the state.
        // KWin doesn't talk to KSMServer directly anymore, so this won't deadlock.
        saveSessionCall.waitForFinished();
    }

    const auto pendingClients = clients;

    for (KSMClient *c : pendingClients) {
        c->resetState();

        SmsSaveYourself(c->connection(), saveType, true, SmInteractStyleAny, false);
    }

    qCDebug(KSMSERVER) << "clients should be empty, " << clients.count();

    if (clients.isEmpty())
        completeShutdownOrCheckpoint();
}

void KSMServer::saveCurrentSession()
{
    if (state != Idle)
        return;

    if (currentSession().isEmpty() || currentSession() == QString::fromLocal8Bit(SESSION_PREVIOUS_LOGOUT))
        sessionGroup = QLatin1String("Session: ") + QString::fromLocal8Bit(SESSION_BY_USER);

    state = Checkpoint;

    saveType = SmSaveLocal;
    saveSession = true;
#ifndef NO_LEGACY_SESSION_MANAGEMENT
    performLegacySessionSave();
#endif

    auto aboutToSaveCall = m_kwinInterface->aboutToSaveSession(currentSession());
    aboutToSaveCall.waitForFinished();

    const auto pendingClients = clients;
    for (KSMClient *c : pendingClients) {
        c->resetState();
        SmsSaveYourself(c->connection(), saveType, false, SmInteractStyleNone, false);
    }
    if (clients.isEmpty())
        completeShutdownOrCheckpoint();
}

void KSMServer::saveCurrentSessionAs(const QString &session)
{
    if (state != Idle)
        return;
    sessionGroup = QStringLiteral("Session: ") + session;
    saveCurrentSession();
}

// callbacks
void KSMServer::saveYourselfDone(KSMClient *client, bool success)
{
    if (state == Idle) {
        // State saving when it's not shutdown or checkpoint. Probably
        // a shutdown was canceled and the client is finished saving
        // only now. Discard the saved state in order to avoid
        // the saved data building up.
        QStringList discard = client->discardCommand();
        if (!discard.isEmpty())
            executeCommand(discard);
        return;
    }
    if (success) {
        client->saveYourselfDone = true;
        completeShutdownOrCheckpoint();
    } else {
        // fake success to make KDE's logout not block with broken
        // apps. A perfect ksmserver would display a warning box at
        // the very end.
        client->saveYourselfDone = true;
        completeShutdownOrCheckpoint();
    }
    startProtection();
}

void KSMServer::interactRequest(KSMClient *client, int /*dialogType*/)
{
    if (state == Shutdown || state == ClosingSubSession)
        client->pendingInteraction = true;
    else
        SmsInteract(client->connection());

    handlePendingInteractions();
}

void KSMServer::interactDone(KSMClient *client, bool cancelShutdown_)
{
    if (client != clientInteracting)
        return; // should not happen
    clientInteracting = nullptr;
    if (cancelShutdown_)
        cancelShutdown(client);
    else
        handlePendingInteractions();
}

void KSMServer::phase2Request(KSMClient *client)
{
    client->waitForPhase2 = true;
    client->wasPhase2 = true;
    completeShutdownOrCheckpoint();
}

void KSMServer::handlePendingInteractions()
{
    if (clientInteracting)
        return;

    foreach (KSMClient *c, clients) {
        if (c->pendingInteraction) {
            clientInteracting = c;
            c->pendingInteraction = false;
            break;
        }
    }
    if (clientInteracting) {
        endProtection();
        SmsInteract(clientInteracting->connection());
    } else {
        startProtection();
    }
}

void KSMServer::cancelShutdown(KSMClient *c)
{
    clientInteracting = nullptr;
    qCDebug(KSMSERVER) << state;
    if (state == ClosingSubSession) {
        clientsToKill.clear();
        clientsToSave.clear();
        Q_EMIT subSessionCloseCanceled();
    } else {
        qCDebug(KSMSERVER) << "Client " << c->program() << " (" << c->clientId() << ") canceled shutdown.";
        KNotification::event(QStringLiteral("cancellogout"), i18n("Logout canceled by '%1'", c->program()), QPixmap(), nullptr, KNotification::DefaultEvent);
        foreach (KSMClient *c, clients) {
            SmsShutdownCancelled(c->connection());
            if (c->saveYourselfDone) {
                // Discard also saved state.
                QStringList discard = c->discardCommand();
                if (!discard.isEmpty())
                    executeCommand(discard);
            }
        }
    }
    state = Idle;

    m_kwinInterface->setState(KWinSessionState::Normal);

    Q_EMIT logoutFinished(false);
}

void KSMServer::startProtection()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration(); // config may have changed in the KControl module
    KConfigGroup cg(config, "General");

    int timeout = cg.readEntry("clientShutdownTimeoutSecs", 15) * 1000;

    protectionTimer.setSingleShot(true);
    protectionTimer.start(timeout);
}

void KSMServer::endProtection()
{
    protectionTimer.stop();
}

/*
Internal protection slot, invoked when clients do not react during
shutdown.
*/
void KSMServer::protectionTimeout()
{
    if ((state != Shutdown && state != Checkpoint && state != ClosingSubSession) || clientInteracting)
        return;

    foreach (KSMClient *c, clients) {
        if (!c->saveYourselfDone && !c->waitForPhase2) {
            qCDebug(KSMSERVER) << "protectionTimeout: client " << c->program() << "(" << c->clientId() << ")";
            c->saveYourselfDone = true;
        }
    }
    completeShutdownOrCheckpoint();
    startProtection();
}

void KSMServer::completeShutdownOrCheckpoint()
{
    qCDebug(KSMSERVER) << "completeShutdownOrCheckpoint called";
    if (state != Shutdown && state != Checkpoint && state != ClosingSubSession)
        return;

    QList<KSMClient *> pendingClients;
    if (state == ClosingSubSession)
        pendingClients = clientsToSave;
    else
        pendingClients = clients;

    foreach (KSMClient *c, pendingClients) {
        if (!c->saveYourselfDone && !c->waitForPhase2)
            return; // not done yet
    }

    // do phase 2
    bool waitForPhase2 = false;
    foreach (KSMClient *c, pendingClients) {
        if (!c->saveYourselfDone && c->waitForPhase2) {
            c->waitForPhase2 = false;
            SmsSaveYourselfPhase2(c->connection());
            waitForPhase2 = true;
        }
    }
    if (waitForPhase2)
        return;

    if (saveSession)
        storeSession();
    else
        discardSession();

    qCDebug(KSMSERVER) << "state is " << state;
    if (state == Shutdown) {
        KNotification *n = KNotification::event(QStringLiteral("exitkde"), QString(), QPixmap(), nullptr, KNotification::DefaultEvent); // Plasma says good bye
        connect(n, &KNotification::closed, this, &KSMServer::startKilling);
        state = WaitingForKNotify;
        // https://bugs.kde.org/show_bug.cgi?id=228005
        // if sound is not working for some reason (e.g. no phonon
        // backends are installed) the closed() signal never happens
        // and logoutSoundFinished() never gets called. Add this timer to make
        // sure the shutdown procedure continues even if sound system is broken.
        QTimer::singleShot(5000, this, [=] {
            if (state == WaitingForKNotify) {
                n->deleteLater();
                startKilling();
            }
        });
    } else if (state == Checkpoint) {
        foreach (KSMClient *c, clients) {
            SmsSaveComplete(c->connection());
        }
        state = Idle;
    } else { // ClosingSubSession
        startKillingSubSession();
    }
}

void KSMServer::startKilling()
{
    qCDebug(KSMSERVER) << "Starting killing clients";
    if (state == Killing) {
        // we are already killing
        return;
    }
    // kill all clients
    state = Killing;

    m_kwinInterface->setState(KWinSessionState::Quitting);

    foreach (KSMClient *c, clients) {
        qCDebug(KSMSERVER) << "startKilling: client " << c->program() << "(" << c->clientId() << ")";
        SmsDie(c->connection());
    }

    qCDebug(KSMSERVER) << " We killed all clients. We have now clients.count()=" << clients.count() << Qt::endl;
    completeKilling();
    QTimer::singleShot(10000, this, &KSMServer::timeoutQuit);
}

void KSMServer::completeKilling()
{
    qCDebug(KSMSERVER) << "KSMServer::completeKilling clients.count()=" << clients.count() << Qt::endl;
    if (state == Killing) {
        if (!clients.isEmpty()) // still waiting for clients to go away
            return;
        killingCompleted();
    }
}

// shutdown is fully complete
void KSMServer::killingCompleted()
{
    Q_EMIT logoutFinished(true);
    qApp->quit();
}

void KSMServer::timeoutQuit()
{
    foreach (KSMClient *c, clients) {
        qCWarning(KSMSERVER) << "SmsDie timeout, client " << c->program() << "(" << c->clientId() << ")";
    }
    killingCompleted();
}

void KSMServer::saveSubSession(const QString &name, QStringList saveAndClose, QStringList saveOnly)
{
    if (state != Idle) { // performing startup
        qCDebug(KSMSERVER) << "not idle!" << state;
        return;
    }
    qCDebug(KSMSERVER) << name << saveAndClose << saveOnly;
    state = ClosingSubSession;
    saveType = SmSaveBoth; // both or local? what oes it mean?
    saveSession = true;
    sessionGroup = QStringLiteral("SubSession: ") + name;

#ifndef NO_LEGACY_SESSION_MANAGEMENT
    // performLegacySessionSave(); FIXME
#endif

    startProtection();
    foreach (KSMClient *c, clients) {
        if (saveAndClose.contains(QString::fromLocal8Bit(c->clientId()))) {
            c->resetState();
            SmsSaveYourself(c->connection(), saveType, true, SmInteractStyleAny, false);
            clientsToSave << c;
            clientsToKill << c;
        } else if (saveOnly.contains(QString::fromLocal8Bit(c->clientId()))) {
            c->resetState();
            SmsSaveYourself(c->connection(), saveType, true, SmInteractStyleAny, false);
            clientsToSave << c;
        }
    }
    completeShutdownOrCheckpoint();
}

void KSMServer::startKillingSubSession()
{
    qCDebug(KSMSERVER) << "Starting killing clients";
    // kill all clients
    state = KillingSubSession;
    foreach (KSMClient *c, clientsToKill) {
        qCDebug(KSMSERVER) << "completeShutdown: client " << c->program() << "(" << c->clientId() << ")";
        SmsDie(c->connection());
    }

    qCDebug(KSMSERVER) << " We killed some clients. We have now clients.count()=" << clients.count() << Qt::endl;
    completeKillingSubSession();
    QTimer::singleShot(10000, this, &KSMServer::signalSubSessionClosed);
}

void KSMServer::completeKillingSubSession()
{
    qCDebug(KSMSERVER) << "KSMServer::completeKillingSubSession clients.count()=" << clients.count() << Qt::endl;
    if (state == KillingSubSession) {
        if (!clientsToKill.isEmpty()) {
            return; // still waiting for clients to go away
        }
        signalSubSessionClosed();
    }
}

void KSMServer::signalSubSessionClosed()
{
    if (state != KillingSubSession)
        return;
    clientsToKill.clear();
    clientsToSave.clear();
    // TODO tell the subSession manager the close request was carried out
    // so that plasma can close its stuff
    state = Idle;
    qCDebug(KSMSERVER) << state;
    Q_EMIT subSessionClosed();
}
