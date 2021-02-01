/*****************************************************************

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

#include "debug.h"

#include "kcminit_interface.h"
#include "kded_interface.h"
#include "ksmserver_interface.h"
#include <klauncher_interface.h>

#include <KCompositeJob>
#include <KConfigGroup>
#include <KIO/DesktopExecParser>
#include <KNotifyConfig>
#include <KProcess>
#include <KService>
#include <Kdelibs4Migration>

#include <phonon/audiooutput.h>
#include <phonon/mediaobject.h>
#include <phonon/mediasource.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include "startupadaptor.h"

#include "../config-startplasma.h"

class Phase : public KCompositeJob
{
    Q_OBJECT
public:
    Phase(const AutoStart &autostart, QObject *parent)
        : KCompositeJob(parent)
        , m_autostart(autostart)
    {
    }

    bool addSubjob(KJob *job) override
    {
        bool rc = KCompositeJob::addSubjob(job);
        job->start();
        return rc;
    }

    void slotResult(KJob *job) override
    {
        KCompositeJob::slotResult(job);
        if (!hasSubjobs()) {
            emitResult();
        }
    }

protected:
    const AutoStart m_autostart;
};

class StartupPhase0 : public Phase
{
    Q_OBJECT
public:
    StartupPhase0(const AutoStart &autostart, QObject *parent)
        : Phase(autostart, parent)
    {
    }
    void start() override
    {
        qCDebug(PLASMA_SESSION) << "Phase 0";
        addSubjob(new AutoStartAppsJob(m_autostart, 0));
        addSubjob(new KCMInitJob());
        addSubjob(new SleepJob());
    }
};

class StartupPhase1 : public Phase
{
    Q_OBJECT
public:
    StartupPhase1(const AutoStart &autostart, QObject *parent)
        : Phase(autostart, parent)
    {
    }
    void start() override
    {
        qCDebug(PLASMA_SESSION) << "Phase 1";
        addSubjob(new AutoStartAppsJob(m_autostart, 1));
    }
};

class StartupPhase2 : public Phase
{
    Q_OBJECT
public:
    StartupPhase2(const AutoStart &autostart, QObject *parent)
        : Phase(autostart, parent)
    {
    }
    void runUserAutostart();
    bool migrateKDE4Autostart(const QString &folder);

    void start() override
    {
        qCDebug(PLASMA_SESSION) << "Phase 2";
        addSubjob(new AutoStartAppsJob(m_autostart, 2));
        addSubjob(new KDEDInitJob());
        runUserAutostart();
    }
};

SleepJob::SleepJob()
{
}

void SleepJob::start()
{
    auto t = new QTimer(this);
    connect(t, &QTimer::timeout, this, [this]() {
        emitResult();
    });
    t->start(100);
}

// Put the notification in its own thread as it can happen that
// PulseAudio will start initializing with this, so let's not
// block the main thread with waiting for PulseAudio to start
class NotificationThread : public QThread
{
    Q_OBJECT
    void run() override
    {
        // We cannot parent to the thread itself so let's create
        // a QObject on the stack and parent everythign to it
        QObject parent;
        KNotifyConfig notifyConfig(QStringLiteral("plasma_workspace"), QList<QPair<QString, QString>>(), QStringLiteral("startkde"));
        const QString action = notifyConfig.readEntry(QStringLiteral("Action"));
        if (action.isEmpty() || !action.split(QLatin1Char('|')).contains(QLatin1String("Sound"))) {
            // no startup sound configured
            return;
        }
        Phonon::AudioOutput *m_audioOutput = new Phonon::AudioOutput(Phonon::NotificationCategory, &parent);

        QString soundFilename = notifyConfig.readEntry(QStringLiteral("Sound"));
        if (soundFilename.isEmpty()) {
            qCWarning(PLASMA_SESSION) << "Audio notification requested, but no sound file provided in notifyrc file, aborting audio notification";
            return;
        }

        QUrl soundURL;
        const auto dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        for (const QString &dataLocation : dataLocations) {
            soundURL = QUrl::fromUserInput(soundFilename, dataLocation + QStringLiteral("/sounds"), QUrl::AssumeLocalFile);
            if (soundURL.isLocalFile() && QFile::exists(soundURL.toLocalFile())) {
                break;
            } else if (!soundURL.isLocalFile() && soundURL.isValid()) {
                break;
            }
            soundURL.clear();
        }
        if (soundURL.isEmpty()) {
            qCWarning(PLASMA_SESSION) << "Audio notification requested, but sound file from notifyrc file was not found, aborting audio notification";
            return;
        }

        Phonon::MediaObject *m = new Phonon::MediaObject(&parent);
        connect(m, &Phonon::MediaObject::finished, this, &NotificationThread::quit);

        Phonon::createPath(m, m_audioOutput);

        m->setCurrentSource(soundURL);
        m->play();
        exec();
    }

private:
    // Prevent application exit until the thread (and hence the sound) completes
    QEventLoopLocker m_locker;
};

Startup::Startup(QObject *parent)
    : QObject(parent)
{
    new StartupAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Startup"), QStringLiteral("org.kde.Startup"), this);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Startup"));

    const AutoStart autostart;

    QProcess::execute(QStringLiteral(CMAKE_INSTALL_FULL_LIBEXECDIR_KF5 "/start_kdeinit_wrapper"), QStringList());

    KJob *phase1;
    QProcessEnvironment kdedProcessEnv;
    kdedProcessEnv.insert(QStringLiteral("KDED_STARTED_BY_KDEINIT"), QStringLiteral("1"));

    KJob *windowManagerJob = nullptr;

    if (qEnvironmentVariable("XDG_SESSION_TYPE") != QLatin1String("wayland")) {
        QString windowManager;
        if (qEnvironmentVariableIsSet("KDEWM")) {
            windowManager = qEnvironmentVariable("KDEWM");
        }
        if (windowManager.isEmpty()) {
            windowManager = QStringLiteral(KWIN_BIN);
        }

        if (windowManager == QLatin1String(KWIN_BIN)) {
            windowManagerJob = new StartServiceJob(windowManager, {}, QStringLiteral("org.kde.KWin"));
        } else {
            windowManagerJob = new StartServiceJob(windowManager, {}, {});
        }
    }

    const QVector<KJob *> sequence = {
        new StartProcessJob(QStringLiteral("kcminit_startup"), {}),
        new StartServiceJob(QStringLiteral("kded5"), {}, QStringLiteral("org.kde.kded5"), kdedProcessEnv),
        windowManagerJob,
        new StartServiceJob(QStringLiteral("ksmserver"), QCoreApplication::instance()->arguments().mid(1), QStringLiteral("org.kde.ksmserver")),
        new StartupPhase0(autostart, this),
        phase1 = new StartupPhase1(autostart, this),
        new RestoreSessionJob(),
        new StartupPhase2(autostart, this),
    };
    KJob *last = nullptr;
    for (KJob *job : sequence) {
        if (!job) {
            continue;
        }
        if (last) {
            connect(last, &KJob::finished, job, &KJob::start);
        }
        last = job;
    }

    connect(phase1, &KJob::finished, this, []() {
        NotificationThread *loginSound = new NotificationThread();
        connect(loginSound, &NotificationThread::finished, loginSound, &NotificationThread::deleteLater);
        loginSound->start();
    });

    connect(sequence.last(), &KJob::finished, this, &Startup::finishStartup);
    sequence.first()->start();

    // app will be closed when all KJobs finish thanks to the QEventLoopLocker in each KJob
}

void Startup::upAndRunning(const QString &msg)
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
    qCDebug(PLASMA_SESSION) << "Finished";
    upAndRunning(QStringLiteral("ready"));
}

void Startup::updateLaunchEnv(const QString &key, const QString &value)
{
    qputenv(key.toLatin1(), value.toLatin1());
}

KCMInitJob::KCMInitJob()
    : KJob()
{
}

void KCMInitJob::start()
{
    org::kde::KCMInit kcminit(QStringLiteral("org.kde.kcminit"), QStringLiteral("/kcminit"), QDBusConnection::sessionBus());
    kcminit.setTimeout(10 * 1000);

    QDBusPendingReply<void> pending = kcminit.runPhase1();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this]() {
        emitResult();
    });
    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
}

KDEDInitJob::KDEDInitJob()
{
}

void KDEDInitJob::start()
{
    qCDebug(PLASMA_SESSION());
    org::kde::kded5 kded(QStringLiteral("org.kde.kded5"), QStringLiteral("/kded"), QDBusConnection::sessionBus());
    auto pending = kded.loadSecondPhase();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this]() {
        emitResult();
    });
    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
}

RestoreSessionJob::RestoreSessionJob()
    : KJob()
{
}

void RestoreSessionJob::start()
{
    OrgKdeKSMServerInterfaceInterface ksmserverIface(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
    auto pending = ksmserverIface.restoreSession();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pending, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this]() {
        emitResult();
    });
    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
}

void StartupPhase2::runUserAutostart()
{
    // Now let's execute the scripts in the KDE-specific autostart-scripts folder.
    const QString autostartFolder =
        QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QDir::separator() + QStringLiteral("autostart-scripts");

    QDir dir(autostartFolder);
    if (!dir.exists()) {
        // Create dir in all cases, so that users can find it :-)
        dir.mkpath(QStringLiteral("."));

        if (!migrateKDE4Autostart(autostartFolder)) {
            return;
        }
    }

    const QStringList entries = dir.entryList(QDir::Files);
    for (const QString &file : entries) {
        // Don't execute backup files
        if (!file.endsWith(QLatin1Char('~')) && !file.endsWith(QLatin1String(".bak")) && (file[0] != QLatin1Char('%') || !file.endsWith(QLatin1Char('%')))
            && (file[0] != QLatin1Char('#') || !file.endsWith(QLatin1Char('#')))) {
            const QString fullPath = dir.absolutePath() + QLatin1Char('/') + file;

            qCInfo(PLASMA_SESSION) << "Starting autostart script " << fullPath;
            auto p = new KProcess; // deleted in onFinished lambda
            p->setProgram(fullPath);
            p->start();
            connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [p](int exitCode) {
                qCInfo(PLASMA_SESSION) << "autostart script" << p->program() << "finished with exit code " << exitCode;
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
    qCDebug(PLASMA_SESSION) << "Copying autostart files from" << oldFolder.path();
    const QStringList entries = oldFolder.entryList(QDir::Files);
    for (const QString &file : entries) {
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
            qCWarning(PLASMA_SESSION) << "Error copying" << src << "to" << dest;
        }
    }
    return true;
}

AutoStartAppsJob::AutoStartAppsJob(const AutoStart &autostart, int phase)
    : m_autoStart(autostart)
{
    m_autoStart.setPhase(phase);
}

void AutoStartAppsJob::start()
{
    qCDebug(PLASMA_SESSION);

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
                qCWarning(PLASMA_SESSION) << "failed to parse" << serviceName << "for autostart";
                continue;
            }
            qCInfo(PLASMA_SESSION) << "Starting autostart service " << serviceName << arguments;
            auto program = arguments.takeFirst();
            if (!QProcess::startDetached(program, arguments))
                qCWarning(PLASMA_SESSION) << "could not start" << serviceName << ":" << program << arguments;
        } while (true);
    });
}

StartServiceJob::StartServiceJob(const QString &process, const QStringList &args, const QString &serviceId, const QProcessEnvironment &additionalEnv)
    : KJob()
    , m_process(new QProcess(this))
    , m_serviceId(serviceId)
    , m_additionalEnv(additionalEnv)
{
    m_process->setProgram(process);
    m_process->setArguments(args);

    auto watcher = new QDBusServiceWatcher(serviceId, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForRegistration, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &StartServiceJob::emitResult);
}

void StartServiceJob::start()
{
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(m_additionalEnv);
    m_process->setProcessEnvironment(env);

    if (!m_serviceId.isEmpty() && QDBusConnection::sessionBus().interface()->isServiceRegistered(m_serviceId)) {
        qCDebug(PLASMA_SESSION) << m_process << "already running";
        emitResult();
        return;
    }
    qCDebug(PLASMA_SESSION) << "Starting " << m_process->program() << m_process->arguments();
    if (!m_process->startDetached()) {
        qCWarning(PLASMA_SESSION) << "error starting process" << m_process->program() << m_process->arguments();
        emitResult();
    }

    if (m_serviceId.isEmpty()) {
        emitResult();
    }
}

StartProcessJob::StartProcessJob(const QString &process, const QStringList &args, const QProcessEnvironment &additionalEnv)
    : KJob()
    , m_process(new QProcess(this))
{
    m_process->setProgram(process);
    m_process->setArguments(args);
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert(additionalEnv);
    m_process->setProcessEnvironment(env);

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this](int exitCode) {
        qCInfo(PLASMA_SESSION) << "process job " << m_process->program() << "finished with exit code " << exitCode;
        emitResult();
    });
}

void StartProcessJob::start()
{
    qCDebug(PLASMA_SESSION) << "Starting " << m_process->program() << m_process->arguments();

    m_process->start();
}

#include "startup.moc"
