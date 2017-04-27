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

#include <QDir>
#include <Kdelibs4Migration>
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

#include <QTimer>
#include <QtDBus/QtDBus>
#include <phonon/audiooutput.h>
#include <phonon/mediaobject.h>
#include <phonon/mediasource.h>
#include <QStandardPaths>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <kio/desktopexecparser.h>
#include <KSharedConfig>
#include <kprocess.h>
#include <KNotifyConfig>
#include <KService>

#include "global.h"
#include "server.h"
#include "client.h"

#include <QX11Info>

//#include "kdesktop_interface.h"
#include <klauncher_interface.h>
#include <qstandardpaths.h>
#include "kcminit_interface.h"

//#define KSMSERVER_STARTUP_DEBUG1

#ifdef KSMSERVER_STARTUP_DEBUG1
static QTime t;
#endif

// Put the notification in its own thread as it can happen that
// PulseAudio will start initializing with this, so let's not
// block the main thread with waiting for PulseAudio to start
class NotificationThread : public QThread
{
    Q_OBJECT
    void run() Q_DECL_OVERRIDE {
        // We cannot parent to the thread itself so let's create
        // a QObject on the stack and parent everythign to it
        QObject parent;
        KNotifyConfig notifyConfig(QStringLiteral("plasma_workspace"), QList< QPair<QString,QString> >(), QStringLiteral("startkde"));
        const QString action = notifyConfig.readEntry(QStringLiteral("Action"));
        if (action.isEmpty() || !action.split('|').contains(QStringLiteral("Sound"))) {
            // no startup sound configured
            return;
        }
        Phonon::AudioOutput *m_audioOutput = new Phonon::AudioOutput(Phonon::NotificationCategory, &parent);

        QString soundFilename = notifyConfig.readEntry(QStringLiteral("Sound"));
        if (soundFilename.isEmpty()) {
            qWarning() << "Audio notification requested, but no sound file provided in notifyrc file, aborting audio notification";
            return;
        }

        QUrl soundURL = QUrl(soundFilename); // this CTOR accepts both absolute paths (/usr/share/sounds/blabla.ogg and blabla.ogg) w/o screwing the scheme
        if (soundURL.isRelative() && !soundURL.toString().startsWith('/')) { // QUrl considers url.scheme.isEmpty() == url.isRelative()
            soundURL = QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("sounds/") + soundFilename));

            if (soundURL.isEmpty()) {
                qWarning() << "Audio notification requested, but sound file from notifyrc file was not found, aborting audio notification";
                return;
            }
        }

        Phonon::MediaObject *m = new Phonon::MediaObject(&parent);
        connect(m, &Phonon::MediaObject::finished, this, &NotificationThread::quit);

        Phonon::createPath(m, m_audioOutput);

        m->setCurrentSource(soundURL);
        m->play();
        exec();
    }

};

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

    qCDebug(KSMSERVER) << "KSMServer::restoreSession " << sessionName;
    KSharedConfig::Ptr config = KSharedConfig::openConfig();

    sessionGroup = QStringLiteral("Session: ") + sessionName;
    KConfigGroup configSessionGroup( config, sessionGroup);

    int count =  configSessionGroup.readEntry( "count", 0 );
    appsToStart = count;
    upAndRunning( QStringLiteral( "ksmserver" ) );

    // find all commands to launch the wm in the session
    QList<QStringList> wmStartCommands;
    if ( !wm.isEmpty() ) {
        for ( int i = 1; i <= count; i++ ) {
            QString n = QString::number(i);
            if ( isWM( configSessionGroup.readEntry( QStringLiteral("program")+n, QString())) ) {
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
    launchWM( QList< QStringList >() << wmCommands );
}

void KSMServer::launchWM( const QList< QStringList >& wmStartCommands )
{
    assert( state == LaunchingWM );

    if (!(qEnvironmentVariableIsSet("WAYLAND_DISPLAY") || qEnvironmentVariableIsSet("WAYLAND_SOCKET"))) {
        // when we have a window manager, we start it first and give
        // it some time before launching other processes. Results in a
        // visually more appealing startup.
        wmProcess = startApplication( wmStartCommands[ 0 ], QString(), QString(), true );
        connect( wmProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(wmProcessChange()));
        connect( wmProcess, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(wmProcessChange()));
    }
    autoStart0();
}

void KSMServer::clientSetProgram( KSMClient* client )
{
    if( client->program() == wm )
        autoStart0();
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
        qWarning() << "Window manager" << wm << "failed to launch";
        if( wm == QStringLiteral( KWIN_BIN ) )
            return; // uhoh, kwin itself failed
        qCDebug(KSMSERVER) << "Launching KWin";
        wm = QStringLiteral( KWIN_BIN );
        wmCommands = ( QStringList() << QStringLiteral( KWIN_BIN ) );
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
    qCDebug(KSMSERVER) << t.elapsed();
#endif

   autoStart(0);
}

void KSMServer::autoStart0Done()
{
    if( state != AutoStart0 )
        return;
    if( !checkStartupSuspend())
        return;
    qCDebug(KSMSERVER) << "Autostart 0 done";
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER) << t.elapsed();
#endif

    state = KcmInitPhase1;
    kcminitSignals = new QDBusInterface( QStringLiteral( "org.kde.kcminit"),
                                         QStringLiteral( "/kcminit" ),
                                         QStringLiteral( "org.kde.KCMInit" ),
                                         QDBusConnection::sessionBus(), this );
    if( !kcminitSignals->isValid()) {
        qWarning() << "kcminit not running? If we are running with mobile profile or in another platform other than X11 this is normal.";
        delete kcminitSignals;
        kcminitSignals = 0;
        QTimer::singleShot(0, this, &KSMServer::kcmPhase1Done);
        return;
    }
    connect( kcminitSignals, SIGNAL(phase1Done()), SLOT(kcmPhase1Done()));
    QTimer::singleShot( 10000, this, &KSMServer::kcmPhase1Timeout); // protection

    org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"),
                              QStringLiteral("/kcminit"),
                              QDBusConnection::sessionBus());
    kcminit.runPhase1();
}

void KSMServer::kcmPhase1Done()
{
    if( state != KcmInitPhase1 )
        return;
    qCDebug(KSMSERVER) << "Kcminit phase 1 done";
    if (kcminitSignals) {
        disconnect( kcminitSignals, SIGNAL(phase1Done()), this, SLOT(kcmPhase1Done()));
    }
    autoStart1();
}

void KSMServer::kcmPhase1Timeout()
{
    if( state != KcmInitPhase1 )
        return;
    qCDebug(KSMSERVER) << "Kcminit phase 1 timeout";
    kcmPhase1Done();
}

void KSMServer::autoStart1()
{
    if( state != KcmInitPhase1 )
        return;
    state = AutoStart1;
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< t.elapsed();
#endif
    autoStart(1);
}

void KSMServer::autoStart1Done()
{
    if( state != AutoStart1 )
        return;
    if( !checkStartupSuspend())
        return;
    qCDebug(KSMSERVER) << "Autostart 1 done";
    setupShortcuts(); // done only here, because it needs kglobalaccel :-/
    lastAppStarted = 0;
    lastIdStarted.clear();
    state = Restoring;
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< t.elapsed();
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
        if ( isWM( config.readEntry( QStringLiteral("program")+n, QString())) )
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
    qCDebug(KSMSERVER)<< t.elapsed();
#endif
    waitAutoStart2 = true;
    waitKcmInit2 = true;
    autoStart(2);
    QTimer::singleShot( 10000, this, &KSMServer::autoStart2Done); //In case klauncher never returns

    QDBusInterface kded( QStringLiteral( "org.kde.kded5" ),
                         QStringLiteral( "/kded" ),
                         QStringLiteral( "org.kde.kded5" ) );
    kded.call( QStringLiteral( "loadSecondPhase" ) );

#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< "kded" << t.elapsed();
#endif

    runUserAutostart();

    if (kcminitSignals) {
        connect( kcminitSignals, SIGNAL(phase2Done()), SLOT(kcmPhase2Done()));
        QTimer::singleShot( 10000, this, &KSMServer::kcmPhase2Timeout); // protection
        org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"),
                                  QStringLiteral("/kcminit"),
                                  QDBusConnection::sessionBus());
        kcminit.runPhase2();
    } else {
        QTimer::singleShot(0, this, &KSMServer::kcmPhase2Done);
    }
    if( !defaultSession())
        restoreLegacySession(KSharedConfig::openConfig().data());

    qCDebug(KSMSERVER) << "Starting notification thread";
    NotificationThread *loginSound = new NotificationThread();
    // Delete the thread when finished
    connect(loginSound, &NotificationThread::finished, loginSound, &NotificationThread::deleteLater);
    loginSound->start();
}

void KSMServer::runUserAutostart()
{
    // Now let's execute the scripts in the KDE-specific autostart-scripts folder.
    const QString autostartFolder = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QDir::separator() + QStringLiteral("autostart-scripts");

    QDir dir(autostartFolder);
    if (!dir.exists()) {
        // Create dir in all cases, so that users can find it :-)
        dir.mkpath(QStringLiteral("."));

        if (!migrateKDE4Autostart(autostartFolder)) {
            return;
        }
    }

    const QStringList entries = dir.entryList(QDir::Files);
    foreach (const QString &file, entries) {
        // Don't execute backup files
        if (!file.endsWith(QLatin1Char('~')) && !file.endsWith(QStringLiteral(".bak")) &&
                (file[0] != QLatin1Char('%') || !file.endsWith(QLatin1Char('%'))) &&
                (file[0] != QLatin1Char('#') || !file.endsWith(QLatin1Char('#'))))
        {
            const QString fullPath = dir.absolutePath() + QLatin1Char('/') + file;

            qCInfo(KSMSERVER) << "Starting autostart script " << fullPath;
            auto p = new KProcess; //deleted in onFinished lambda
            p->setProgram(fullPath);
            p->start();
            connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), [p](int exitCode) {
                qCInfo(KSMSERVER) << "autostart script" << p->program() << "finished with exit code " << exitCode;
                p->deleteLater();
            });
        }
    }
}

bool KSMServer::migrateKDE4Autostart(const QString &autostartFolder)
{
    // Migrate user autostart from kde4
    Kdelibs4Migration migration;
    if (!migration.kdeHomeFound()) {
        return false;
    }
    // KDEHOME/Autostart was the default value for KGlobalSettings::autostart()
    QString oldAutostart = migration.kdeHome() + QStringLiteral("/Autostart");
    // That path could be customized in kdeglobals
    const QString oldKdeGlobals = migration.locateLocal("config", QStringLiteral("kdeglobals"));
    if (!oldKdeGlobals.isEmpty()) {
        oldAutostart = KConfig(oldKdeGlobals).group("Paths").readEntry("Autostart", oldAutostart);
    }

    const QDir oldFolder(oldAutostart);
    qCDebug(KSMSERVER) << "Copying autostart files from" << oldFolder.path();
    const QStringList entries = oldFolder.entryList(QDir::Files);
    foreach (const QString &file, entries) {
        const QString src = oldFolder.absolutePath() + QLatin1Char('/') + file;
        const QString dest = autostartFolder + QLatin1Char('/') + file;
        QFileInfo info(src);
        bool success;
        if (info.isSymLink()) {
            // This will only work with absolute symlink targets
            success = QFile::link(info.symLinkTarget(), dest);
        } else {
            success = QFile::copy(src, dest);
        }
        if (!success) {
            qCWarning(KSMSERVER) << "Error copying" << src << "to" << dest;
        }
    }
    return true;
}

void KSMServer::autoStart2Done()
{
    if( state != FinishingStartup )
        return;
    qCDebug(KSMSERVER) << "Autostart 2 done";
    waitAutoStart2 = false;
    finishStartup();
}

void KSMServer::kcmPhase2Done()
{
    if( state != FinishingStartup )
        return;
    qCDebug(KSMSERVER) << "Kcminit phase 2 done";
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
    qCDebug(KSMSERVER) << "Kcminit phase 2 timeout";
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
    qCDebug(KSMSERVER)<< t.elapsed();
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
    qCDebug(KSMSERVER) << "Startup suspend timeout:" << state;
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
            qWarning() << "Unknown resume startup state" ;
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


void KSMServer::autoStart(int phase)
{
    if (m_autoStart.phase() >= phase) {
        return;
    }
    m_autoStart.setPhase(phase);
    if (phase == 0) {
        m_autoStart.loadAutoStartList();
    }
    QTimer::singleShot(0, this, &KSMServer::slotAutoStart);
}

void KSMServer::slotAutoStart()
{
    do {
        QString serviceName = m_autoStart.startService();
        if (serviceName.isEmpty()) {
            // Done
            if (!m_autoStart.phaseDone()) {
                m_autoStart.setPhaseDone();
                switch (m_autoStart.phase()) {
                case 0:
                    autoStart0Done();
                    break;
                case 1:
                    autoStart1Done();
                    break;
                case 2:
                    autoStart2Done();
                    break;
                }
            }
            return;
        }
        KService service(serviceName);
        qCInfo(KSMSERVER) << "Starting autostart service " << serviceName;
        auto p = new KProcess(this);
        auto arguments = KIO::DesktopExecParser(service, QList<QUrl>()).resultingArguments();
        if (arguments.isEmpty()) {
            qCInfo(KSMSERVER) << "failed to parse" << serviceName << "for autostart";
            continue;
        }
        auto program = arguments.takeFirst();
        p->setProgram(program, arguments);
        p->start();
        connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), [p](int exitCode) {
            qCInfo(KSMSERVER) << "autostart service" << p->program() << "finished with exit code " << exitCode;
            p->deleteLater();
        });
    } while (true);
    // Loop till we find a service that we can start.
}

#include "startup.moc"
