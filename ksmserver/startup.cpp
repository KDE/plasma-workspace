/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>
Copyright 2005 Lubos Lunak <l.lunak@kde.org>

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

#include <kglobalsettings.h>
#include <QDir>
#include <krun.h>
#include <config-workspace.h>
#include <config-unix.h> // HAVE_LIMITS_H

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

#include <QPushButton>
#include <QTimer>
#include <QtDBus/QtDBus>

#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kapplication.h>
#include <ktemporaryfile.h>
#include <knotification.h>
#include <kconfiggroup.h>
#include <kprocess.h>

#include "global.h"
#include "server.h"
#include "client.h"
#include <kdebug.h>

#include <QX11Info>

//#include "kdesktop_interface.h"
#include <klauncher_interface.h>
#include <qstandardpaths.h>
#include "kcminit_interface.h"

//#define KSMSERVER_STARTUP_DEBUG1

#ifdef KSMSERVER_STARTUP_DEBUG1
static QTime t;
#endif

/*!  Restores the previous session. Ensures the window manager is
  running (if specified).
 */
void KSMServer::restoreSession( const QString &sessionName )
{
    if( state != Idle )
        return;
#ifdef KSMSERVER_STARTUP_DEBUG1
    t.start();
#endif
    state = LaunchingWM;

    kDebug( 1218 ) << "KSMServer::restoreSession " << sessionName;
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    sessionGroup = QStringLiteral("Session: ") + sessionName;
    KConfigGroup configSessionGroup( config, sessionGroup);

    int count =  configSessionGroup.readEntry( "count", 0 );
    appsToStart = count;
    upAndRunning( QStringLiteral( "ksmserver" ) );
    connect( klauncherSignals, SIGNAL(autoStart0Done()), SLOT(autoStart0Done()));
    connect( klauncherSignals, SIGNAL(autoStart1Done()), SLOT(autoStart1Done()));
    connect( klauncherSignals, SIGNAL(autoStart2Done()), SLOT(autoStart2Done()));

    // find all commands to launch the wm in the session
    QList<QStringList> wmStartCommands;
    if ( !wm.isEmpty() ) {
        for ( int i = 1; i <= count; i++ ) {
            QString n = QString::number(i);
            if ( wm == configSessionGroup.readEntry( QStringLiteral("program")+n, QString() ) ) {
                wmStartCommands << configSessionGroup.readEntry( QStringLiteral("restartCommand")+n, QStringList() );
            }
        }
    } 
    if( wmStartCommands.isEmpty()) // otherwise use the configured default
        wmStartCommands << wmCommands;

    launchWM( wmStartCommands );
}

/*!
  Starts the default session.

  Currently, that's the window manager only (if specified).
 */
void KSMServer::startDefaultSession()
{
    if( state != Idle )
        return;
    state = LaunchingWM;
#ifdef KSMSERVER_STARTUP_DEBUG1
    t.start();
#endif
    sessionGroup = QString();
    upAndRunning( QStringLiteral( "ksmserver" ) );
    connect( klauncherSignals, SIGNAL(autoStart0Done()), SLOT(autoStart0Done()));
    connect( klauncherSignals, SIGNAL(autoStart1Done()), SLOT(autoStart1Done()));
    connect( klauncherSignals, SIGNAL(autoStart2Done()), SLOT(autoStart2Done()));

    launchWM( QList< QStringList >() << wmCommands );
}

void KSMServer::launchWM( const QList< QStringList >& wmStartCommands )
{
    assert( state == LaunchingWM );

    // when we have a window manager, we start it first and give
    // it some time before launching other processes. Results in a
    // visually more appealing startup.
    wmProcess = startApplication( wmStartCommands[ 0 ], QString(), QString(), true );
    connect( wmProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(wmProcessChange()));
    connect( wmProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(wmProcessChange()));
    //Let's try to remove this and see how smooth things are nowadays
//     QTimer::singleShot( 4000, this, SLOT(autoStart0()) );
    autoStart0();
}

void KSMServer::clientSetProgram( KSMClient* client )
{
    if( client->program() == wm )
        autoStart0();
#ifndef KSMSERVER_STARTUP_DEBUGl
    if( state == Idle )
        {
        static int cnt = 0;
        if( client->program() == QStringLiteral( "gedit" ) && ( cnt == 0 ))
            ++cnt;
        else if( client->program() == QStringLiteral( "konqueror" ) && ( cnt == 1 ))
            ++cnt;
        else if( client->program() == QStringLiteral( "kspaceduel" ) && ( cnt == 2 ))
            ++cnt;
        else if( client->program() == QStringLiteral( "gedit" ) && ( cnt == 3 ))
            ++cnt;
        else
            cnt = 0;
        if( cnt == 4 )
            KMessageBox::information( NULL, QStringLiteral( "drat" ) );
        }
#endif
}

void KSMServer::wmProcessChange()
{
    if( state != LaunchingWM )
    { // don't care about the process when not in the wm-launching state anymore
        wmProcess = NULL;
        return;
    }
    if( wmProcess->state() == QProcess::NotRunning )
    { // wm failed to launch for some reason, go with kwin instead
        kWarning( 1218 ) << "Window manager" << wm << "failed to launch";
        if( wm == QStringLiteral( "kwin" ) )
            return; // uhoh, kwin itself failed
        kDebug( 1218 ) << "Launching KWin";
        wm = QStringLiteral( "kwin" );
        wmCommands = ( QStringList() << QStringLiteral( "kwin" ) );
        // launch it
        launchWM( QList< QStringList >() << wmCommands );
        return;
    }
}

void KSMServer::autoStart0()
{
    if( state != LaunchingWM )
        return;
    if( !checkStartupSuspend())
        return;
    state = AutoStart0;
#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << t.elapsed();
#endif
    org::kde::KLauncher klauncher(QStringLiteral("org.kde.klauncher5"), QStringLiteral("/KLauncher"), QDBusConnection::sessionBus());
    klauncher.autoStart((int)0);
}

void KSMServer::autoStart0Done()
{
    if( state != AutoStart0 )
        return;
    disconnect( klauncherSignals, SIGNAL(autoStart0Done()), this, SLOT(autoStart0Done()));
    if( !checkStartupSuspend())
        return;
    kDebug( 1218 ) << "Autostart 0 done";
#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << t.elapsed();
#endif
    upAndRunning( QStringLiteral( "desktop" ) );
    state = KcmInitPhase1;
    kcminitSignals = new QDBusInterface( QStringLiteral( "org.kde.kcminit"),
                                         QStringLiteral( "/kcminit" ),
                                         QStringLiteral( "org.kde.KCMInit" ),
                                         QDBusConnection::sessionBus(), this );
    if( !kcminitSignals->isValid()) {
        kWarning() << "kcminit not running? If we are running with mobile profile or in another platform other than X11 this is normal.";
        delete kcminitSignals;
        kcminitSignals = 0;
        QTimer::singleShot(0, this, SLOT(kcmPhase1Done()));
        return;
    }
    connect( kcminitSignals, SIGNAL(phase1Done()), SLOT(kcmPhase1Done()));
    QTimer::singleShot( 10000, this, SLOT(kcmPhase1Timeout())); // protection

    org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"),
                              QStringLiteral("/kcminit"),
                              QDBusConnection::sessionBus());
    kcminit.runPhase1();
}

void KSMServer::kcmPhase1Done()
{
    if( state != KcmInitPhase1 )
        return;
    kDebug( 1218 ) << "Kcminit phase 1 done";
    if (kcminitSignals) {
        disconnect( kcminitSignals, SIGNAL(phase1Done()), this, SLOT(kcmPhase1Done()));
    }
    autoStart1();
}

void KSMServer::kcmPhase1Timeout()
{
    if( state != KcmInitPhase1 )
        return;
    kDebug( 1218 ) << "Kcminit phase 1 timeout";
    kcmPhase1Done();
}

void KSMServer::autoStart1()
{
    if( state != KcmInitPhase1 )
        return;
    state = AutoStart1;
#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << t.elapsed();
#endif
    org::kde::KLauncher klauncher(QStringLiteral("org.kde.klauncher5"),
                                  QStringLiteral("/KLauncher"),
                                  QDBusConnection::sessionBus());
    klauncher.autoStart((int)1);
}

void KSMServer::autoStart1Done()
{
    if( state != AutoStart1 )
        return;
    disconnect( klauncherSignals, SIGNAL(autoStart1Done()), this, SLOT(autoStart1Done()));
    if( !checkStartupSuspend())
        return;
    kDebug( 1218 ) << "Autostart 1 done";
    setupShortcuts(); // done only here, because it needs kglobalaccel :-/
    lastAppStarted = 0;
    lastIdStarted.clear();
    state = Restoring;
#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << t.elapsed();
#endif
    if( defaultSession()) {
        autoStart2();
        return;
    }
    tryRestoreNext();
}

void KSMServer::clientRegistered( const char* previousId )
{
    if ( previousId && lastIdStarted == QString::fromLocal8Bit( previousId ) )
        tryRestoreNext();
}

void KSMServer::tryRestoreNext()
{
    if( state != Restoring && state != RestoringSubSession )
        return;
    restoreTimer.stop();
    startupSuspendTimeoutTimer.stop();
    KConfigGroup config(KSharedConfig::openConfig(), sessionGroup );

    while ( lastAppStarted < appsToStart ) {
        lastAppStarted++;
        QString n = QString::number(lastAppStarted);
        QString clientId = config.readEntry( QStringLiteral("clientId")+n, QString() );
        bool alreadyStarted = false;
        foreach ( KSMClient *c, clients ) {
            if ( QString::fromLocal8Bit( c->clientId() ) == clientId ) {
                alreadyStarted = true;
                break;
            }
        }
        if ( alreadyStarted )
            continue;

        QStringList restartCommand = config.readEntry( QStringLiteral("restartCommand")+n, QStringList() );
        if ( restartCommand.isEmpty() ||
             (config.readEntry( QStringLiteral("restartStyleHint")+n, 0 ) == SmRestartNever)) {
            continue;
        }
        if ( wm == config.readEntry( QStringLiteral("program")+n, QString() ) )
            continue; // wm already started
        if( config.readEntry( QStringLiteral( "wasWm" )+n, false ))
            continue; // it was wm before, but not now, don't run it (some have --replace in command :(  )
        startApplication( restartCommand,
                          config.readEntry( QStringLiteral("clientMachine")+n, QString() ),
                          config.readEntry( QStringLiteral("userId")+n, QString() ));
        lastIdStarted = clientId;
        if ( !lastIdStarted.isEmpty() ) {
            restoreTimer.setSingleShot( true );
            restoreTimer.start( 2000 );
            return; // we get called again from the clientRegistered handler
        }
    }

    //all done
    appsToStart = 0;
    lastIdStarted.clear();

    if (state == Restoring)
        autoStart2();
    else { //subsession
        state = Idle;
        emit subSessionOpened();
    }
}

void KSMServer::autoStart2()
{
    if( state != Restoring )
        return;
    if( !checkStartupSuspend())
        return;
    state = FinishingStartup;
#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << t.elapsed();
#endif
    waitAutoStart2 = true;
    waitKcmInit2 = true;
    org::kde::KLauncher klauncher(QStringLiteral("org.kde.klauncher5"),
                                  QStringLiteral("/KLauncher"),
                                  QDBusConnection::sessionBus());
    klauncher.autoStart((int)2);

#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << "klauncher" << t.elapsed();
#endif

    QDBusInterface kded( QStringLiteral( "org.kde.kded5" ),
                         QStringLiteral( "/kded" ),
                         QStringLiteral( "org.kde.kded5" ) );
    kded.call( QStringLiteral( "loadSecondPhase" ) );

#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << "kded" << t.elapsed();
#endif

    runUserAutostart();

    if (kcminitSignals) {
        connect( kcminitSignals, SIGNAL(phase2Done()), SLOT(kcmPhase2Done()));
        QTimer::singleShot( 10000, this, SLOT(kcmPhase2Timeout())); // protection
        org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"),
                                  QStringLiteral("/kcminit"),
                                  QDBusConnection::sessionBus());
        kcminit.runPhase2();
    } else {
        QTimer::singleShot(0, this, SLOT(kcmPhase2Done()));
    }
    if( !defaultSession())
        restoreLegacySession(KSharedConfig::openConfig().data());
//     KNotification::event( QStringLiteral( "startkde" ),
//                           QString(), QPixmap(), 0l,
//                           KNotification::DefaultEvent ); // this is the time KDE is up, more or less
}

void KSMServer::runUserAutostart()
{
    // now let's execute all the stuff in the autostart folder.
    // the stuff will actually be really executed when the event loop is
    // entered, since KRun internally uses a QTimer
    QDir dir(QStandardPaths::locate( QStandardPaths::GenericConfigLocation, QStringLiteral("autostart"), QStandardPaths::LocateDirectory));
    if (dir.exists()) {
        const QStringList entries = dir.entryList( QDir::Files );
        foreach (const QString& file, entries) {
            // Don't execute backup files
            if ( !file.endsWith( QLatin1Char( '~' ) ) && !file.endsWith( QStringLiteral( ".bak" ) ) &&
                 ( file[0] != QLatin1Char( '%' ) || !file.endsWith( QLatin1Char( '%' ) ) ) &&
                 ( file[0] != QLatin1Char( '#' ) || !file.endsWith( QLatin1Char( '#' ) ) ) )
            {
                QUrl url = QUrl::fromLocalFile( dir.absolutePath() + QLatin1Char( '/' ) + file );
                //KRun is synchronous so if we use it here it will produce a deadlock.
                //So isntead we use kioclient for now.
                //(void) new KRun( url, 0, true );
                QProcess::startDetached(QStringLiteral("kioclient"),
                    QStringList()
                    << QStringLiteral("exec")
                    << url.path()
                );
            }
        }
    } else {
        // Create dir so that users can find it :-)
        dir.mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                   + QDir::separator() + QStringLiteral("autostart"));
    }
}

void KSMServer::autoStart2Done()
{
    if( state != FinishingStartup )
        return;
    disconnect( klauncherSignals, SIGNAL(autoStart2Done()), this, SLOT(autoStart2Done()));
    kDebug( 1218 ) << "Autostart 2 done";
    waitAutoStart2 = false;
    finishStartup();
}

void KSMServer::kcmPhase2Done()
{
    if( state != FinishingStartup )
        return;
    kDebug( 1218 ) << "Kcminit phase 2 done";
    if (kcminitSignals) {
        disconnect( kcminitSignals, SIGNAL(phase2Done()), this, SLOT(kcmPhase2Done()));
        delete kcminitSignals;
        kcminitSignals = 0;
    }
    waitKcmInit2 = false;
    finishStartup();
}

void KSMServer::kcmPhase2Timeout()
{
    if( !waitKcmInit2 )
        return;
    kDebug( 1218 ) << "Kcminit phase 2 timeout";
    kcmPhase2Done();
}

void KSMServer::finishStartup()
{
    if( state != FinishingStartup )
        return;
    if( waitAutoStart2 || waitKcmInit2 )
        return;

    upAndRunning( QStringLiteral( "ready" ) );
#ifdef KSMSERVER_STARTUP_DEBUG1
    kDebug() << t.elapsed();
#endif

    state = Idle;
    setupXIOErrorHandler(); // From now on handle X errors as normal shutdown.
}

bool KSMServer::checkStartupSuspend()
{
    if( startupSuspendCount.isEmpty())
        return true;
    // wait for the phase to finish
    if( !startupSuspendTimeoutTimer.isActive())
    {
        startupSuspendTimeoutTimer.setSingleShot( true );
        startupSuspendTimeoutTimer.start( 10000 );
    }
    return false;
}

void KSMServer::suspendStartup( const QString &app )
{
    if( !startupSuspendCount.contains( app ))
        startupSuspendCount[ app ] = 0;
    ++startupSuspendCount[ app ];
}

void KSMServer::resumeStartup( const QString &app )
{
    if( !startupSuspendCount.contains( app ))
        return;
    if( --startupSuspendCount[ app ] == 0 ) {
        startupSuspendCount.remove( app );
        if( startupSuspendCount.isEmpty() && startupSuspendTimeoutTimer.isActive()) {
            startupSuspendTimeoutTimer.stop();
            resumeStartupInternal();
        }
    }
}

void KSMServer::startupSuspendTimeout()
{
    kDebug( 1218 ) << "Startup suspend timeout:" << state;
    resumeStartupInternal();
}

void KSMServer::resumeStartupInternal()
{
    startupSuspendCount.clear();
    switch( state ) {
        case LaunchingWM:
            autoStart0();
          break;
        case AutoStart0:
            autoStart0Done();
          break;
        case AutoStart1:
            autoStart1Done();
          break;
        case Restoring:
            autoStart2();
          break;
        default:
            kWarning( 1218 ) << "Unknown resume startup state" ;
          break;
    }
}

void KSMServer::upAndRunning( const QString& msg )
{
    QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                                                         QStringLiteral("/KSplash"),
                                                                         QStringLiteral("org.kde.KSplash"),
                                                                         QStringLiteral("setStage"));
    ksplashProgressMessage.setArguments(QList<QVariant>() << msg);
    QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
}

void KSMServer::restoreSubSession( const QString& name )
{
    sessionGroup = QStringLiteral( "SubSession: " ) + name;

    KConfigGroup configSessionGroup( KSharedConfig::openConfig(), sessionGroup);
    int count =  configSessionGroup.readEntry( "count", 0 );
    appsToStart = count;
    lastAppStarted = 0;
    lastIdStarted.clear();

    state = RestoringSubSession;
    tryRestoreNext();
}
