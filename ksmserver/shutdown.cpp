/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>

relatively small extensions by Oswald Buddenhagen <ob6@inf.tu-dresden.de>

some code taken from the dcopserver (part of the KDE libraries), which is
Copyright 1999 Matthias Ettrich <ettrich@kde.org>
Copyright 1999 Preston Brown <pbrown@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/


#include <config-workspace.h>
#include <config-unix.h> // HAVE_LIMITS_H
#include <config-ksmserver.h>

#include <ksmserver_debug.h>

#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <QApplication>
#include <QPushButton>
#include <QTimer>
#include <QtDBus/QtDBus>
#include <QtConcurrentRun>

#include <KConfig>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KUserTimestamp>
#include <KNotification>
#include <kdisplaymanager.h>
#include "server.h"
#include "global.h"
#include "client.h"

#include <solid/powermanagement.h>

#include <QDesktopWidget>
#include <QX11Info>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

void KSMServer::logout( int confirm, int sdtype, int sdmode )
{
    // KDE5: remove me
    if (sdtype == KWorkSpace::ShutdownTypeLogout)
        sdtype = KWorkSpace::ShutdownTypeNone;

    shutdown( (KWorkSpace::ShutdownConfirm)confirm,
            (KWorkSpace::ShutdownType)sdtype,
            (KWorkSpace::ShutdownMode)sdmode );
}

bool KSMServer::canShutdown()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration(); // config may have changed in the KControl module
    KConfigGroup cg( config, "General");

    return cg.readEntry( "offerShutdown", true ) && KDisplayManager().canShutdown();
}

bool KSMServer::isShuttingDown() const
{
    return state >= Shutdown;
}

bool readFromPipe(int pipe)
{
    QFile readPipe;
    if (!readPipe.open(pipe, QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray result = readPipe.readLine();
    if (result.isEmpty()) {
        return false;
    }
    bool ok = false;
    const int number = result.toInt(&ok);
    if (!ok) {
        return false;
    }
    KSMServer::self()->shutdownType = KWorkSpace::ShutdownType(number);

    return true;
}

void KSMServer::shutdown( KWorkSpace::ShutdownConfirm confirm,
    KWorkSpace::ShutdownType sdtype, KWorkSpace::ShutdownMode sdmode )
{
	qDebug() << "Shutdown called with confirm " << confirm
			 << " type " << sdtype << " and mode " << sdmode;
    pendingShutdown.stop();
    if( dialogActive )
        return;
    if( state >= Shutdown ) // already performing shutdown
        return;
    if( state != Idle ) // performing startup
    {
    // perform shutdown as soon as startup is finished, in order to avoid saving partial session
        if( !pendingShutdown.isActive())
        {
            pendingShutdown.start( 1000 );
            pendingShutdown_confirm = confirm;
            pendingShutdown_sdtype = sdtype;
            pendingShutdown_sdmode = sdmode;
        }
        return;
    }

    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration(); // config may have changed in the KControl module

	KConfigGroup cg( config, "General");

    bool logoutConfirmed =
        (confirm == KWorkSpace::ShutdownConfirmYes) ? false :
    (confirm == KWorkSpace::ShutdownConfirmNo) ? true :
                !cg.readEntry( "confirmLogout", true );
    bool choose = false;
    bool maysd = false;
    if (cg.readEntry( "offerShutdown", true ) && KDisplayManager().canShutdown())
        maysd = true;
    if (!maysd) {
        if (sdtype != KWorkSpace::ShutdownTypeNone &&
            sdtype != KWorkSpace::ShutdownTypeDefault &&
            logoutConfirmed)
            return; /* unsupported fast shutdown */
        sdtype = KWorkSpace::ShutdownTypeNone;
    } else if (sdtype == KWorkSpace::ShutdownTypeDefault) {
        sdtype = (KWorkSpace::ShutdownType)
                cg.readEntry( "shutdownType", (int)KWorkSpace::ShutdownTypeNone );
        choose = true;
    }
    if (sdmode == KWorkSpace::ShutdownModeDefault)
        sdmode = KWorkSpace::ShutdownModeInteractive;

	qDebug() << "After modifications confirm is " << confirm
			 << " type is " << sdtype << " and mode " << sdmode;
    QString bopt;
    if ( !logoutConfirmed ) {
        int pipeFds[2];
        if (pipe(pipeFds) != 0) {
            return;
        }
        QProcess *p = new QProcess(this);
        p->setProgram(QStringLiteral(LOGOUT_GREETER_BIN));
        QStringList arguments;
        if (maysd) {
            arguments << QStringLiteral("--shutdown-allowed");
        }
        if (choose) {
            arguments << QStringLiteral("--choose");
        }
        if (sdtype != KWorkSpace::ShutdownTypeDefault) {
            arguments << QStringLiteral("--mode");
            switch (sdtype) {
            case KWorkSpace::ShutdownTypeHalt:
                arguments << QStringLiteral("shutdown");
                break;
            case KWorkSpace::ShutdownTypeReboot:
                arguments << QStringLiteral("reboot");
                break;
            case KWorkSpace::ShutdownTypeNone:
            default:
                // logout
                arguments << QStringLiteral("logout");
                break;
            }
        }
        arguments << QStringLiteral("--mode-fd");
        arguments << QString::number(pipeFds[1]);
        p->setArguments(arguments);

        const int resultPipe = pipeFds[0];
        connect(p, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::error), this,
            [this, resultPipe] {
                close(resultPipe);
                dialogActive = false;
            }
        );

        connect(p, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this,
            [this, resultPipe, sdmode, p] (int exitCode) {
                p->deleteLater();
                dialogActive = false;
                if (exitCode != 0) {
                    close(resultPipe);
                    return;
                }
                QFutureWatcher<bool> *watcher = new QFutureWatcher<bool>();
                QObject::connect(watcher, &QFutureWatcher<bool>::finished, this,
                    [this, sdmode, watcher] {
                        const bool result = watcher->result();
                        if (!result) {
                            // it failed to read, don't logout
                            return;
                        }
                        shutdownMode = sdmode;
                        bootOption = QString();
                        performLogout();
                    }, Qt::QueuedConnection);
                QObject::connect(watcher, &QFutureWatcher<void>::finished, watcher, &QFutureWatcher<bool>::deleteLater, Qt::QueuedConnection);
                watcher->setFuture(QtConcurrent::run(readFromPipe, resultPipe));
            }
        );

        dialogActive = true;
        p->start();
        close(pipeFds[1]);
    } else {
        shutdownType = sdtype;
        shutdownMode = sdmode;
        bootOption = bopt;

        performLogout();
    }
}

void KSMServer::performLogout()
{
    // If the logout was confirmed, let's start a powermanagement inhibition.
    // We store the cookie so we can interrupt it if the logout will be canceled
    inhibitCookie = Solid::PowerManagement::beginSuppressingSleep(QStringLiteral("Shutting down system"));

    // shall we save the session on logout?
    KConfigGroup cg(KSharedConfig::openConfig(), "General");
    saveSession = ( cg.readEntry( "loginMode",
                                    QStringLiteral( "restorePreviousLogout" ) )
                    == QStringLiteral( "restorePreviousLogout" ) );

    qDebug() << "saveSession is " << saveSession;

    if ( saveSession )
        sessionGroup = QStringLiteral( "Session: " ) + QString::fromLocal8Bit( SESSION_PREVIOUS_LOGOUT );

    // Set the real desktop background to black so that exit looks
    // clean regardless of what was on "our" desktop.
            QPalette palette;
    palette.setColor( QApplication::desktop()->backgroundRole(), Qt::black );
    QApplication::setPalette(palette);
    state = Shutdown;
    wmPhase1WaitingCount = 0;
    saveType = saveSession?SmSaveBoth:SmSaveGlobal;
#ifndef NO_LEGACY_SESSION_MANAGEMENT
    performLegacySessionSave();
#endif
    startProtection();
    foreach( KSMClient* c, clients ) {
        c->resetState();
        // Whoever came with the idea of phase 2 got it backwards
        // unfortunately. Window manager should be the very first
        // one saving session data, not the last one, as possible
        // user interaction during session save may alter
        // window positions etc.
        // Moreover, KWin's focus stealing prevention would lead
        // to undesired effects while session saving (dialogs
        // wouldn't be activated), so it needs be assured that
        // KWin will turn it off temporarily before any other
        // user interaction takes place.
        // Therefore, make sure the WM finishes its phase 1
        // before others a chance to change anything.
        // KWin will check if the session manager is ksmserver,
        // and if yes it will save in phase 1 instead of phase 2.
        if( isWM( c ) )
            ++wmPhase1WaitingCount;
    }
    if (wmPhase1WaitingCount > 0) {
        foreach( KSMClient* c, clients ) {
            if( isWM( c ) )
                SmsSaveYourself( c->connection(), saveType,
                            true, SmInteractStyleAny, false );
        }
    } else { // no WM, simply start them all
        foreach( KSMClient* c, clients )
            SmsSaveYourself( c->connection(), saveType,
                        true, SmInteractStyleAny, false );
    }
    qDebug() << "clients should be empty, " << clients.isEmpty();
    if ( clients.isEmpty() )
        completeShutdownOrCheckpoint();
    dialogActive = false;
}

void KSMServer::pendingShutdownTimeout()
{
    shutdown( pendingShutdown_confirm, pendingShutdown_sdtype, pendingShutdown_sdmode );
}

void KSMServer::saveCurrentSession()
{
    if ( state != Idle || dialogActive )
        return;

    if ( currentSession().isEmpty() || currentSession() == QString::fromLocal8Bit( SESSION_PREVIOUS_LOGOUT ) )
        sessionGroup = QStringLiteral("Session: ") + QString::fromLocal8Bit( SESSION_BY_USER );

    state = Checkpoint;
    wmPhase1WaitingCount = 0;
    saveType = SmSaveLocal;
    saveSession = true;
#ifndef NO_LEGACY_SESSION_MANAGEMENT
    performLegacySessionSave();
#endif
    foreach( KSMClient* c, clients ) {
        c->resetState();
        if( isWM( c ) )
            ++wmPhase1WaitingCount;
    }
    if (wmPhase1WaitingCount > 0) {
        foreach( KSMClient* c, clients ) {
            if( isWM( c ) )
                SmsSaveYourself( c->connection(), saveType, false, SmInteractStyleNone, false );
        }
    } else {
        foreach( KSMClient* c, clients )
            SmsSaveYourself( c->connection(), saveType, false, SmInteractStyleNone, false );
    }
    if ( clients.isEmpty() )
        completeShutdownOrCheckpoint();
}

void KSMServer::saveCurrentSessionAs( const QString &session )
{
    if ( state != Idle || dialogActive )
        return;
    sessionGroup = QStringLiteral( "Session: " ) + session;
    saveCurrentSession();
}

// callbacks
void KSMServer::saveYourselfDone( KSMClient* client, bool success )
{
    if ( state == Idle ) {
        // State saving when it's not shutdown or checkpoint. Probably
        // a shutdown was canceled and the client is finished saving
        // only now. Discard the saved state in order to avoid
        // the saved data building up.
        QStringList discard = client->discardCommand();
        if( !discard.isEmpty())
            executeCommand( discard );
        return;
    }
    if ( success ) {
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
    if( isWM( client ) && !client->wasPhase2 && wmPhase1WaitingCount > 0 ) {
        --wmPhase1WaitingCount;
        // WM finished its phase1, save the rest
        if( wmPhase1WaitingCount == 0 ) {
            foreach( KSMClient* c, clients )
                if( !isWM( c ))
                    SmsSaveYourself( c->connection(), saveType, saveType != SmSaveLocal,
                        saveType != SmSaveLocal ? SmInteractStyleAny : SmInteractStyleNone,
                        false );
        }
    }
}

void KSMServer::interactRequest( KSMClient* client, int /*dialogType*/ )
{
    if ( state == Shutdown || state == ClosingSubSession )
        client->pendingInteraction = true;
    else
        SmsInteract( client->connection() );

    handlePendingInteractions();
}

void KSMServer::interactDone( KSMClient* client, bool cancelShutdown_ )
{
    if ( client != clientInteracting )
        return; // should not happen
    clientInteracting = 0;
    if ( cancelShutdown_ )
        cancelShutdown( client );
    else
        handlePendingInteractions();
}


void KSMServer::phase2Request( KSMClient* client )
{
    client->waitForPhase2 = true;
    client->wasPhase2 = true;
    completeShutdownOrCheckpoint();
    if( isWM( client ) && wmPhase1WaitingCount > 0 ) {
        --wmPhase1WaitingCount;
        // WM finished its phase1 and requests phase2, save the rest
        if( wmPhase1WaitingCount == 0 ) {
            foreach( KSMClient* c, clients )
                if( !isWM( c ))
                    SmsSaveYourself( c->connection(), saveType, saveType != SmSaveLocal,
                        saveType != SmSaveLocal ? SmInteractStyleAny : SmInteractStyleNone,
                        false );
        }
    }
}

void KSMServer::handlePendingInteractions()
{
    if ( clientInteracting )
        return;

    foreach( KSMClient* c, clients ) {
        if ( c->pendingInteraction ) {
            clientInteracting = c;
            c->pendingInteraction = false;
            break;
        }
    }
    if ( clientInteracting ) {
        endProtection();
        SmsInteract( clientInteracting->connection() );
    } else {
        startProtection();
    }
}


void KSMServer::cancelShutdown( KSMClient* c )
{
    clientInteracting = 0;
    qCDebug(KSMSERVER) << state;
    if ( state == ClosingSubSession ) {
        clientsToKill.clear();
        clientsToSave.clear();
        emit subSessionCloseCanceled();
    } else {
        Solid::PowerManagement::stopSuppressingSleep(inhibitCookie);
        qCDebug(KSMSERVER) << "Client " << c->program() << " (" << c->clientId() << ") canceled shutdown.";
//         KNotification::event( QStringLiteral( "cancellogout" ),
//                               i18n( "Logout canceled by '%1'", c->program()),
//                               QPixmap() , 0l , KNotification::DefaultEvent  );
        foreach( KSMClient* c, clients ) {
            SmsShutdownCancelled( c->connection() );
            if( c->saveYourselfDone ) {
                // Discard also saved state.
                QStringList discard = c->discardCommand();
                if( !discard.isEmpty())
                    executeCommand( discard );
            }
        }
    }
    state = Idle;
}

void KSMServer::startProtection()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    config->reparseConfiguration(); // config may have changed in the KControl module
    KConfigGroup cg( config, "General" );

    int timeout = cg.readEntry( "clientShutdownTimeoutSecs", 15 ) * 1000;

    protectionTimer.setSingleShot( true );
    protectionTimer.start( timeout );
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
    if ( ( state != Shutdown && state != Checkpoint && state != ClosingSubSession ) || clientInteracting )
        return;

    foreach( KSMClient* c, clients ) {
        if ( !c->saveYourselfDone && !c->waitForPhase2 ) {
            qCDebug(KSMSERVER) << "protectionTimeout: client " << c->program() << "(" << c->clientId() << ")";
            c->saveYourselfDone = true;
        }
    }
    completeShutdownOrCheckpoint();
    startProtection();
}

void KSMServer::completeShutdownOrCheckpoint()
{
	qDebug() << "completeShutdownOrCheckpoint called";
    if ( state != Shutdown && state != Checkpoint && state != ClosingSubSession )
        return;

    QList<KSMClient*> pendingClients;
    if (state == ClosingSubSession)
        pendingClients = clientsToSave;
    else
        pendingClients = clients;

    foreach( KSMClient* c, pendingClients ) {
        if ( !c->saveYourselfDone && !c->waitForPhase2 )
            return; // not done yet
    }

    // do phase 2
    bool waitForPhase2 = false;
    foreach( KSMClient* c, pendingClients ) {
        if ( !c->saveYourselfDone && c->waitForPhase2 ) {
            c->waitForPhase2 = false;
            SmsSaveYourselfPhase2( c->connection() );
            waitForPhase2 = true;
        }
    }
    if ( waitForPhase2 )
        return;

    if ( saveSession )
        storeSession();
    else
        discardSession();

	qDebug() << "state is " << state;
    if ( state == Shutdown ) {
        KNotification *n = KNotification::event(QStringLiteral("exitkde"), QString(), QPixmap(), 0l,  KNotification::DefaultEvent); // Plasma says good bye
        connect(n, &KNotification::closed, this, &KSMServer::startKilling);
        state = WaitingForKNotify;
        // https://bugs.kde.org/show_bug.cgi?id=228005
        // if sound is not working for some reason (e.g. no phonon
        // backends are installed) the closed() signal never happens
        // and logoutSoundFinished() never gets called. Add this timer to make
        // sure the shutdown procedure continues even if sound system is broken.
        QTimer::singleShot(5000, this, [=]{
            if (state == WaitingForKNotify) {
                n->deleteLater();
                startKilling();
            }
        });
        createLogoutEffectWidget();

    } else if ( state == Checkpoint ) {
        foreach( KSMClient* c, clients ) {
            SmsSaveComplete( c->connection());
        }
        state = Idle;
    } else { //ClosingSubSession
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
    foreach( KSMClient* c, clients ) {
        if( isWM( c )) // kill the WM as the last one in order to reduce flicker
            continue;
        qCDebug(KSMSERVER) << "completeShutdown: client " << c->program() << "(" << c->clientId() << ")";
        SmsDie( c->connection() );
    }

    qCDebug(KSMSERVER) << " We killed all clients. We have now clients.count()=" <<
    clients.count() << endl;
    completeKilling();
    QTimer::singleShot( 10000, this, &KSMServer::timeoutQuit );
}

void KSMServer::completeKilling()
{
    qCDebug(KSMSERVER) << "KSMServer::completeKilling clients.count()=" <<
        clients.count() << endl;
    if( state == Killing ) {
        bool wait = false;
        foreach( KSMClient* c, clients ) {
            if( isWM( c ))
                continue;
            wait = true; // still waiting for clients to go away
        }
        if( wait )
            return;
        killWM();
    }
}

void KSMServer::killWM()
{
    if( state != Killing )
        return;
    delete logoutEffectWidget;

    qCDebug(KSMSERVER) << "Starting killing WM";
    state = KillingWM;
    bool iswm = false;
    foreach( KSMClient* c, clients ) {
        if( isWM( c )) {
            iswm = true;
            qCDebug(KSMSERVER) << "killWM: client " << c->program() << "(" << c->clientId() << ")";
            SmsDie( c->connection() );
        }
    }
    if( iswm ) {
        completeKillingWM();
        QTimer::singleShot( 5000, this, &KSMServer::timeoutWMQuit );
    }
    else
        killingCompleted();
}

void KSMServer::completeKillingWM()
{
    qCDebug(KSMSERVER) << "KSMServer::completeKillingWM clients.count()=" <<
        clients.count() << endl;
    if( state == KillingWM ) {
        if( clients.isEmpty())
            killingCompleted();
    }
}

// shutdown is fully complete
void KSMServer::killingCompleted()
{
    qApp->quit();
}

void KSMServer::timeoutQuit()
{
    foreach( KSMClient* c, clients ) {
        qWarning() << "SmsDie timeout, client " << c->program() << "(" << c->clientId() << ")" ;
    }
    killWM();
}

void KSMServer::timeoutWMQuit()
{
    if( state == KillingWM ) {
        qWarning() << "SmsDie WM timeout" ;
    }
    killingCompleted();
}

void KSMServer::createLogoutEffectWidget()
{
// Ok, this is rather a hack. In order to fade the whole desktop when playing the logout
// sound, killing applications and leaving KDE, create a dummy window that triggers
// the logout fade effect again.
    logoutEffectWidget = new QWidget( NULL, Qt::X11BypassWindowManagerHint );
    logoutEffectWidget->winId(); // workaround for Qt4.3 setWindowRole() assert
    logoutEffectWidget->setWindowRole( QStringLiteral( "logouteffect" ) );

    // Qt doesn't set this on unmanaged windows
    //FIXME: or does it?
    XChangeProperty( QX11Info::display(), logoutEffectWidget->winId(),
        XInternAtom( QX11Info::display(), "WM_WINDOW_ROLE", False ), XA_STRING, 8, PropModeReplace,
        (unsigned char *)"logouteffect", strlen( "logouteffect" ));

    logoutEffectWidget->setGeometry( -100, -100, 1, 1 );
    logoutEffectWidget->show();
}

void KSMServer::saveSubSession(const QString &name, QStringList saveAndClose, QStringList saveOnly)
{
    if( state != Idle ) { // performing startup
        qCDebug(KSMSERVER) << "not idle!" << state;
        return;
    }
    qCDebug(KSMSERVER) << name << saveAndClose << saveOnly;
    state = ClosingSubSession;
    saveType = SmSaveBoth; //both or local? what oes it mean?
    saveSession = true;
    sessionGroup = QStringLiteral( "SubSession: " ) + name;

#ifndef NO_LEGACY_SESSION_MANAGEMENT
    //performLegacySessionSave(); FIXME
#endif

    startProtection();
    foreach( KSMClient* c, clients ) {
        if (saveAndClose.contains(QString::fromLocal8Bit(c->clientId()))) {
            c->resetState();
            SmsSaveYourself( c->connection(), saveType,
                             true, SmInteractStyleAny, false );
            clientsToSave << c;
            clientsToKill << c;
        } else if (saveOnly.contains(QString::fromLocal8Bit(c->clientId()))) {
            c->resetState();
            SmsSaveYourself( c->connection(), saveType,
                             true, SmInteractStyleAny, false );
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
    foreach( KSMClient* c, clientsToKill ) {
        qCDebug(KSMSERVER) << "completeShutdown: client " << c->program() << "(" << c->clientId() << ")";
        SmsDie( c->connection() );
    }

    qCDebug(KSMSERVER) << " We killed some clients. We have now clients.count()=" <<
    clients.count() << endl;
    completeKillingSubSession();
    QTimer::singleShot( 10000, this, &KSMServer::signalSubSessionClosed );
}

void KSMServer::completeKillingSubSession()
{
    qCDebug(KSMSERVER) << "KSMServer::completeKillingSubSession clients.count()=" <<
        clients.count() << endl;
    if( state == KillingSubSession ) {
        bool wait = false;
        foreach( KSMClient* c, clientsToKill ) {
            if( isWM( c ))
                continue;
            wait = true; // still waiting for clients to go away
        }
        if( wait )
            return;
        signalSubSessionClosed();
    }
}

void KSMServer::signalSubSessionClosed()
{
    if( state != KillingSubSession )
        return;
    clientsToKill.clear();
    clientsToSave.clear();
    //TODO tell the subSession manager the close request was carried out
    //so that plasma can close its stuff
    state = Idle;
    qCDebug(KSMSERVER) << state;
    emit subSessionClosed();
}
