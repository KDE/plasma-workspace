/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>
Copyright 2005 Lubos Lunak <l.lunak@kde.org>
Copyright 2018 David Edmundson <davidedmundson@kde.org>


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

#include <config-workspace.h>
#include <config-unix.h> // HAVE_LIMITS_H
#include <config-ksmserver.h>

#include <ksmserver_debug.h>

#include "kcminit_interface.h"
#include <klauncher_interface.h>

#include <KCompositeJob>
#include <Kdelibs4Migration>
#include <KIO/DesktopExecParser>
#include <KJob>
#include <KNotifyConfig>
#include <KProcess>
#include <KService>

#include <phonon/audiooutput.h>
#include <phonon/mediaobject.h>
#include <phonon/mediasource.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDir>
#include <QStandardPaths>
#include <QTimer>
#include <QX11Info>

class Phase: public KCompositeJob
{
Q_OBJECT
public:
    Phase(QObject *parent):
        KCompositeJob(parent)
    {}

    bool addSubjob(KJob *job) override {
        bool rc = KCompositeJob::addSubjob(job);
        job->start();
        return rc;
    }

    void slotResult(KJob *job) override {
        KCompositeJob::slotResult(job);
        if (!hasSubjobs()) {
            emitResult();
        }
    }
};

class StartupPhase0: public Phase
{
Q_OBJECT
public:
    StartupPhase0(QObject *parent) : Phase(parent)
    {}
    void start() override {
        qCDebug(KSMSERVER) << "Phase 0";
        addSubjob(new AutoStartAppsJob(0));
        addSubjob(new KCMInitJob(1));
    }
};

class StartupPhase1: public Phase
{
Q_OBJECT
public:
    StartupPhase1(QObject *parent) : Phase(parent)
    {}
    void start() override {
        qCDebug(KSMSERVER) << "Phase 1";
        addSubjob(new AutoStartAppsJob(1));
    }
};

class StartupPhase2: public Phase
{
Q_OBJECT
public:
    StartupPhase2(QObject *parent) : Phase(parent)
    {}
    void runUserAutostart();
    bool migrateKDE4Autostart(const QString &folder);

    void start() override {
        qCDebug(KSMSERVER) << "Phase 2";
        addSubjob(new AutoStartAppsJob(2));
        addSubjob(new KDEDInitJob());
        addSubjob(new KCMInitJob(2));
        runUserAutostart();
    }
};

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
    ksmserver(parent)
{
    auto phase0 = new StartupPhase0(this);
    auto phase1 = new StartupPhase1(this);
    auto phase2 = new StartupPhase2(this);
    auto restoreSession = new RestoreSessionJob(ksmserver);

    connect(ksmserver, &KSMServer::windowManagerLoaded, phase0, &KJob::start);
    connect(phase0, &KJob::finished, phase1, &KJob::start);

    connect(phase1, &KJob::finished, this, [=]() {
        ksmserver->setupShortcuts(); // done only here, because it needs kglobalaccel :-/
    });

    connect(phase1, &KJob::finished, restoreSession, &KJob::start);
    connect(restoreSession, &KJob::finished, phase2, &KJob::start);

    connect(phase1, &KJob::finished, this, []() {
        NotificationThread *loginSound = new NotificationThread();
        connect(loginSound, &NotificationThread::finished, loginSound, &NotificationThread::deleteLater);
        loginSound->start();});
    connect(phase2, &KJob::finished, this, &Startup::finishStartup);
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

void Startup::finishStartup()
{
    qCDebug(KSMSERVER) << "Finished";
    ksmserver->state = KSMServer::Idle;
    ksmserver->setupXIOErrorHandler();
    upAndRunning(QStringLiteral("ready"));
}

KCMInitJob::KCMInitJob(int phase)
    :m_phase(phase)
{
}

void KCMInitJob::start() {
    //FIXME - replace all this with just a DBus call with a timeout and make kcminit delay the reply till it's done

    auto kcminitSignals = new QDBusInterface( QStringLiteral( "org.kde.kcminit"),
                                         QStringLiteral( "/kcminit" ),
                                         QStringLiteral( "org.kde.KCMInit" ),
                                         QDBusConnection::sessionBus(), this );
    if( !kcminitSignals->isValid()) {
        qCWarning(KSMSERVER) << "kcminit not running? If we are running with mobile profile or in another platform other than X11 this is normal.";
        QTimer::singleShot(0, this, &KCMInitJob::done);
        return;
    }
    if (m_phase == 1) {
        connect( kcminitSignals, SIGNAL(phase1Done()), this, SLOT(done()));
    } else {
        connect( kcminitSignals, SIGNAL(phase2Done()), this, SLOT(done()));
    }
    QTimer::singleShot( 10000, this, &KCMInitJob::done); // protection

    org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"),
                              QStringLiteral("/kcminit"),
                              QDBusConnection::sessionBus());

    if (m_phase == 1) {
        kcminit.runPhase1();
    } else {
        kcminit.runPhase2();
    }
}

void KCMInitJob::done()
{
    emitResult();
}

KDEDInitJob::KDEDInitJob()
{
}

void KDEDInitJob::start() {
    qCDebug(KSMSERVER());
    QDBusInterface kded( QStringLiteral( "org.kde.kded5" ),
                         QStringLiteral( "/kded" ),
                         QStringLiteral( "org.kde.kded5" ) );
    auto pending = kded.asyncCall( QStringLiteral( "loadSecondPhase" ) );

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this]() {emitResult();});
    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
}

RestoreSessionJob::RestoreSessionJob(KSMServer *server): KJob(),
    m_ksmserver(server)
{}

void RestoreSessionJob::start()
{
    if (m_ksmserver->defaultSession()) {
        QTimer::singleShot(0, this, [this]() {emitResult();});
        return;
    }

    m_ksmserver->lastAppStarted = 0;
    m_ksmserver->lastIdStarted.clear();
    m_ksmserver->state = KSMServer::Restoring;
    connect(m_ksmserver, &KSMServer::sessionRestored, this, [this]() {emitResult();});
    m_ksmserver->tryRestoreNext();
}

void StartupPhase2::runUserAutostart()
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

bool StartupPhase2::migrateKDE4Autostart(const QString &autostartFolder)
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

AutoStartAppsJob::AutoStartAppsJob(int phase)
{
    m_autoStart.loadAutoStartList(); //FIXME, share this between jobs
    m_autoStart.setPhase(phase);
}

void AutoStartAppsJob::start() {
    qCDebug(KSMSERVER());

    QTimer::singleShot(0, this, [=]() {
        do {
            QString serviceName = m_autoStart.startService();
            if (serviceName.isEmpty()) {
                // Done
                if (!m_autoStart.phaseDone()) {
                    m_autoStart.setPhaseDone();
                }
                emitResult();
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
    });
}

#include "startup.moc"
