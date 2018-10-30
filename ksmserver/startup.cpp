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

#include "startup.h"
#include "server.h"

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
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
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
    void run() override {
        // We cannot parent to the thread itself so let's create
        // a QObject on the stack and parent everythign to it
        QObject parent;
        KNotifyConfig notifyConfig(QStringLiteral("plasma_workspace"), QList< QPair<QString,QString> >(), QStringLiteral("startkde"));
        const QString action = notifyConfig.readEntry(QStringLiteral("Action"));
        if (action.isEmpty() || !action.split(QLatin1Char('|')).contains(QStringLiteral("Sound"))) {
            // no startup sound configured
            return;
        }
        Phonon::AudioOutput *m_audioOutput = new Phonon::AudioOutput(Phonon::NotificationCategory, &parent);

        QString soundFilename = notifyConfig.readEntry(QStringLiteral("Sound"));
        if (soundFilename.isEmpty()) {
            qCWarning(KSMSERVER) << "Audio notification requested, but no sound file provided in notifyrc file, aborting audio notification";
            return;
        }

        QUrl soundURL;
        const auto dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        for (const QString &dataLocation: dataLocations) {
            soundURL = QUrl::fromUserInput(soundFilename,
                                           dataLocation + QStringLiteral("/sounds"),
                                           QUrl::AssumeLocalFile);
            if (soundURL.isLocalFile() && QFile::exists(soundURL.toLocalFile())) {
                break;
            } else if (!soundURL.isLocalFile() && soundURL.isValid()) {
                break;
            }
            soundURL.clear();
        }
        if (soundURL.isEmpty()) {
            qCWarning(KSMSERVER) << "Audio notification requested, but sound file from notifyrc file was not found, aborting audio notification";
            return;
        }

        Phonon::MediaObject *m = new Phonon::MediaObject(&parent);
        connect(m, &Phonon::MediaObject::finished, this, &NotificationThread::quit);

        Phonon::createPath(m, m_audioOutput);

        m->setCurrentSource(soundURL);
        m->play();
        exec();
    }

};

Startup::Startup(KSMServer *parent):
    QObject(parent),
    ksmserver(parent),
    state(Waiting)
{
    connect(ksmserver, &KSMServer::windowManagerLoaded, this, &Startup::autoStart0);
}

void Startup::autoStart0()
{
    disconnect(ksmserver, &KSMServer::windowManagerLoaded, this, &Startup::autoStart0);
    state = AutoStart0;
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER) << t.elapsed();
#endif

   autoStart(0);
}

void Startup::autoStart0Done()
{
    if( state != AutoStart0 )
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
        qCWarning(KSMSERVER) << "kcminit not running? If we are running with mobile profile or in another platform other than X11 this is normal.";
        delete kcminitSignals;
        kcminitSignals = nullptr;
        QTimer::singleShot(0, this, &Startup::kcmPhase1Done);
        return;
    }
    connect( kcminitSignals, SIGNAL(phase1Done()), SLOT(kcmPhase1Done()));
    QTimer::singleShot( 10000, this, &Startup::kcmPhase1Timeout); // protection

    org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"),
                              QStringLiteral("/kcminit"),
                              QDBusConnection::sessionBus());
    kcminit.runPhase1();
}

void Startup::kcmPhase1Done()
{
    if( state != KcmInitPhase1 )
        return;
    qCDebug(KSMSERVER) << "Kcminit phase 1 done";
    if (kcminitSignals) {
        disconnect( kcminitSignals, SIGNAL(phase1Done()), this, SLOT(kcmPhase1Done()));
    }
    autoStart1();
}

void Startup::kcmPhase1Timeout()
{
    if( state != KcmInitPhase1 )
        return;
    qCDebug(KSMSERVER) << "Kcminit phase 1 timeout";
    kcmPhase1Done();
}

void Startup::autoStart1()
{
    if( state != KcmInitPhase1 )
        return;
    state = AutoStart1;
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< t.elapsed();
#endif
    autoStart(1);
}

void Startup::autoStart1Done()
{
    if( state != AutoStart1 )
        return;
    qCDebug(KSMSERVER) << "Autostart 1 done";
    ksmserver->setupShortcuts(); // done only here, because it needs kglobalaccel :-/
    ksmserver->lastAppStarted = 0;
    ksmserver->lastIdStarted.clear();
    ksmserver->state = KSMServer::Restoring;
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< t.elapsed();
#endif
    if( ksmserver->defaultSession()) {
        autoStart2();
        return;
    }
    ksmserver->tryRestoreNext();
    connect(ksmserver, &KSMServer::sessionRestored, this, &Startup::autoStart2);
}

void Startup::autoStart2()
{
    if( ksmserver->state != KSMServer::Restoring )
        return;
    ksmserver->startupDone();

    state = FinishingStartup;
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< t.elapsed();
#endif
    waitAutoStart2 = true;
    waitKcmInit2 = true;
    autoStart(2);

    QDBusInterface kded( QStringLiteral( "org.kde.kded5" ),
                         QStringLiteral( "/kded" ),
                         QStringLiteral( "org.kde.kded5" ) );
    auto pending = kded.asyncCall( QStringLiteral( "loadSecondPhase" ) );

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, this);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, &Startup::secondKDEDPhaseLoaded);
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
    runUserAutostart();

    if (kcminitSignals) {
        connect( kcminitSignals, SIGNAL(phase2Done()), SLOT(kcmPhase2Done()));
        QTimer::singleShot( 10000, this, &Startup::kcmPhase2Timeout); // protection
        org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"),
                                  QStringLiteral("/kcminit"),
                                  QDBusConnection::sessionBus());
        kcminit.runPhase2();
    } else {
        QTimer::singleShot(0, this, &Startup::kcmPhase2Done);
    }
}

void Startup::secondKDEDPhaseLoaded()
{

#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< "kded" << t.elapsed();
#endif

    if( !ksmserver->defaultSession())
        ksmserver->restoreLegacySession(KSharedConfig::openConfig().data());

    qCDebug(KSMSERVER) << "Starting notification thread";
    NotificationThread *loginSound = new NotificationThread();
    // Delete the thread when finished
    connect(loginSound, &NotificationThread::finished, loginSound, &NotificationThread::deleteLater);
    loginSound->start();
}

void Startup::runUserAutostart()
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

bool Startup::migrateKDE4Autostart(const QString &autostartFolder)
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

void Startup::autoStart2Done()
{
    if( state != FinishingStartup )
        return;
    qCDebug(KSMSERVER) << "Autostart 2 done";
    waitAutoStart2 = false;
    finishStartup();
}

void Startup::kcmPhase2Done()
{
    if( state != FinishingStartup )
        return;
    qCDebug(KSMSERVER) << "Kcminit phase 2 done";
    if (kcminitSignals) {
        disconnect( kcminitSignals, SIGNAL(phase2Done()), this, SLOT(kcmPhase2Done()));
        delete kcminitSignals;
        kcminitSignals = nullptr;
    }
    waitKcmInit2 = false;
    finishStartup();
}

void Startup::kcmPhase2Timeout()
{
    if( !waitKcmInit2 )
        return;
    qCDebug(KSMSERVER) << "Kcminit phase 2 timeout";
    kcmPhase2Done();
}

void Startup::finishStartup()
{
    if( state != FinishingStartup )
        return;
    if( waitAutoStart2 || waitKcmInit2 )
        return;

    upAndRunning( QStringLiteral( "ready" ) );
#ifdef KSMSERVER_STARTUP_DEBUG1
    qCDebug(KSMSERVER)<< t.elapsed();
#endif

    state = Waiting;
    ksmserver->setupXIOErrorHandler(); // From now on handle X errors as normal shutdown.
}

void Startup::upAndRunning( const QString& msg )
{
    QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                                                         QStringLiteral("/KSplash"),
                                                                         QStringLiteral("org.kde.KSplash"),
                                                                         QStringLiteral("setStage"));
    ksplashProgressMessage.setArguments(QList<QVariant>() << msg);
    QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);
}


void Startup::autoStart(int phase)
{
    if (m_autoStart.phase() >= phase) {
        return;
    }
    m_autoStart.setPhase(phase);
    if (phase == 0) {
        m_autoStart.loadAutoStartList();
    }
    QTimer::singleShot(0, this, &Startup::slotAutoStart);
}

void Startup::slotAutoStart()
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
        auto arguments = KIO::DesktopExecParser(service, QList<QUrl>()).resultingArguments();
        if (arguments.isEmpty()) {
            qCWarning(KSMSERVER) << "failed to parse" << serviceName << "for autostart";
            continue;
        }
        qCInfo(KSMSERVER) << "Starting autostart service " << serviceName << arguments;
        auto program = arguments.takeFirst();
        if (!QProcess::startDetached(program, arguments))
            qCWarning(KSMSERVER) << "could not start" << serviceName << ":" << program << arguments;
    } while (true);
    // Loop till we find a service that we can start.
}

#include "startup.moc"
