/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

 Copyright 1999 Martin R. Jones <mjones@kde.org>
 Copyright 2003 Oswald Buddenhagen <ossi@kde.org>
 Copyright 2008 Chani Armitage <chanika@gmail.com>
 Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "ksldapp.h"
#include "interface.h"
#include "lockwindow.h"
#include "logind.h"
#include "kscreensaversettings.h"
#include <config-ksmserver.h>
// workspace
#include <kdisplaymanager.h>
// KDE
#include <KActionCollection>
#include <KAuthorized>
#include <KIdleTime>
#include <KLocalizedString>
// #include <KNotification>
#include <KGlobalAccel>
#include <KCrash>
// Qt
#include <QAction>
#include <QTimer>
#include <QProcess>
#include <QX11Info>
// X11
#include <X11/Xlib.h>
#include <xcb/xcb.h>
// other
#include <unistd.h>
#include <signal.h>

namespace ScreenLocker
{

static KSldApp * s_instance = 0;

KSldApp* KSldApp::self()
{
    if (!s_instance) {
        s_instance = new KSldApp();
    }

    return s_instance;
}

KSldApp::KSldApp(QObject * parent)
    : QObject(parent)
    , m_actionCollection(NULL)
    , m_lockState(Unlocked)
    , m_lockProcess(NULL)
    , m_lockWindow(NULL)
    , m_lockedTimer(QElapsedTimer())
    , m_idleId(0)
    , m_lockGrace(0)
    , m_inGraceTime(false)
    , m_graceTimer(new QTimer(this))
    , m_inhibitCounter(0)
{
    initialize();
}

KSldApp::~KSldApp()
{
}

static int s_XTimeout;
static int s_XInterval;
static int s_XBlanking;
static int s_XExposures;

void KSldApp::cleanUp()
{
    if (m_lockProcess && m_lockProcess->state() != QProcess::NotRunning) {
        m_lockProcess->terminate();
    }
    delete m_actionCollection;
    delete m_lockProcess;
    delete m_lockWindow;

    // Restore X screensaver parameters
    XSetScreenSaver(QX11Info::display(), s_XTimeout, s_XInterval, s_XBlanking, s_XExposures);
}

static bool s_graceTimeKill = false;
static bool s_logindExit = false;

void KSldApp::initialize()
{
    KCrash::setFlags(KCrash::AutoRestart);
    // Save X screensaver parameters
    XGetScreenSaver(QX11Info::display(), &s_XTimeout, &s_XInterval, &s_XBlanking, &s_XExposures);
    // And disable it. The internal X screensaver is not used at all, but we use its
    // internal idle timer (and it is also used by DPMS support in X). This timer must not
    // be altered by this code, since e.g. resetting the counter after activating our
    // screensaver would prevent DPMS from activating. We use the timer merely to detect
    // user activity.
    XSetScreenSaver(QX11Info::display(), 0, s_XInterval, s_XBlanking, s_XExposures);

    // Global keys
    m_actionCollection = new KActionCollection(this);

    if (KAuthorized::authorize(QLatin1String("lock_screen"))) {
        qDebug() << "Configuring Lock Action";
        QAction *a = m_actionCollection->addAction(QLatin1String("Lock Session"));
        a->setText(i18n("Lock Session"));
        KGlobalAccel::self()->setShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::Key_L);
        KGlobalAccel::self()->setDefaultShortcut(a, QList<QKeySequence>() << Qt::ALT+Qt::CTRL+Qt::Key_L);
        connect(a, &QAction::triggered, this,
            [this]() {
                lock(EstablishLock::Immediate);
            }
        );
    }
    m_actionCollection->readSettings();

    // idle support
    auto idleTimeSignal = static_cast<void (KIdleTime:: *)(int)>(&KIdleTime::timeoutReached);
    connect(KIdleTime::instance(), idleTimeSignal, this,
        [this](int identifier) {
            if (identifier != m_idleId) {
                // not our identifier
                return;
            }
            if (lockState() != Unlocked) {
                return;
            }
            if (m_inhibitCounter) {
                // there is at least one process blocking the auto lock of screen locker
                return;
            }
            if (m_lockGrace) {  // short-circuit if grace time is zero
                m_inGraceTime = true;
                m_graceTimer->start(m_lockGrace);
            } else if (m_lockGrace == -1) {
                m_inGraceTime = true;  // if no timeout configured, grace time lasts forever
            }

            lock(EstablishLock::Delayed);
        }
    );

    m_lockProcess = new QProcess();
    m_lockProcess->setReadChannel(QProcess::StandardOutput);
    auto finishedSignal = static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished);
    connect(m_lockProcess, finishedSignal, this,
        [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if ((!exitCode && exitStatus == QProcess::NormalExit) || s_graceTimeKill || s_logindExit) {
                // unlock process finished successfully - we can remove the lock grab
                s_graceTimeKill = false;
                s_logindExit = false;
                doUnlock();
                return;
            }
            // failure, restart lock process
            startLockProcess(EstablishLock::Immediate);
        }
    );
    connect(m_lockProcess, &QProcess::readyReadStandardOutput, this,
        [this]() {
            m_lockState = Locked;
            m_lockedTimer.restart();
            emit locked();
        }
    );
    m_lockedTimer.invalidate();
    m_graceTimer->setSingleShot(true);
    connect(m_graceTimer, &QTimer::timeout, this, &KSldApp::endGraceTime);
    // create our D-Bus interface
    new Interface(this);

    // connect to logind
    LogindIntegration *logind = new LogindIntegration(this);
    connect(logind, &LogindIntegration::requestLock, this,
        [this]() {
            lock(EstablishLock::Immediate);
        }
    );
    connect(logind, &LogindIntegration::requestUnlock, this,
        [this]() {
            if (lockState() == Locked) {
                s_logindExit = true;
                m_lockProcess->kill();
            }
        }
    );

    configure();
}

void KSldApp::configure()
{
    KScreenSaverSettings::self()->readConfig();
    // idle support
    if (m_idleId) {
        KIdleTime::instance()->removeIdleTimeout(m_idleId);
        m_idleId = 0;
    }
    const int timeout = KScreenSaverSettings::timeout();
    // screen saver enabled means there is an auto lock timer
    if (timeout > 0) {
        // timeout stored in seconds
        m_idleId = KIdleTime::instance()->addIdleTimeout(timeout*1000);
    }
    if (KScreenSaverSettings::lock()) {
        m_lockGrace = KScreenSaverSettings::lockGrace();
    } else {
        m_lockGrace = -1;
    }
}

void KSldApp::lock(EstablishLock establishLock)
{
    if (lockState() != Unlocked) {
        // already locked or acquiring lock, no need to lock again
        // but make sure it's really locked
        endGraceTime();
        if (establishLock == EstablishLock::Immediate) {
            // signal the greeter to switch to immediateLock mode
            kill(m_lockProcess->pid(), SIGUSR1);
        }
        return;
    }

    qDebug() << "lock called";
    if (!establishGrab()) {
        qCritical() << "Could not establish screen lock";
        return;
    }

    KDisplayManager().setLock(true);
//     KNotification::event(QLatin1String( "locked" ));

    // blank the screen
    showLockWindow();

    m_lockState = AcquiringLock;

    // start unlock screen process
    if (!startLockProcess(establishLock)) {
        doUnlock();
        qCritical() << "Greeter Process not available";
    }
}

KActionCollection *KSldApp::actionCollection()
{
    return m_actionCollection;
}

/*
 * Forward declarations:
 * Only called from KSldApp::establishGrab(). Using from somewhere else is incorrect usage!
 **/
static bool grabKeyboard();
static bool grabMouse();

bool KSldApp::establishGrab()
{
    XSync(QX11Info::display(), False);

    if (!grabKeyboard()) {
        sleep(1);
        if (!grabKeyboard()) {
            return false;
        }
    }

    if (!grabMouse()) {
        sleep(1);
        if (!grabMouse()) {
            XUngrabKeyboard(QX11Info::display(), CurrentTime);
            return false;
        }
    }

    return true;
}

static bool grabKeyboard()
{
    int rv = XGrabKeyboard( QX11Info::display(), QX11Info::appRootWindow(),
        True, GrabModeAsync, GrabModeAsync, CurrentTime );

    return (rv == GrabSuccess);
}

static bool grabMouse()
{
#define GRABEVENTS ButtonPressMask | ButtonReleaseMask | PointerMotionMask | \
                   EnterWindowMask | LeaveWindowMask
    int rv = XGrabPointer( QX11Info::display(), QX11Info::appRootWindow(),
            True, GRABEVENTS, GrabModeAsync, GrabModeAsync, None,
            None, CurrentTime );
#undef GRABEVENTS

    return (rv == GrabSuccess);
}

void KSldApp::doUnlock()
{
    qDebug() << "Grab Released";
    xcb_connection_t *c = QX11Info::connection();
    xcb_ungrab_keyboard(c, XCB_CURRENT_TIME);
    xcb_ungrab_pointer(c, XCB_CURRENT_TIME);
    xcb_flush(c);
    hideLockWindow();
    // delete the window again, to get rid of event filter
    delete m_lockWindow;
    m_lockWindow = NULL;
    m_lockState = Unlocked;
    m_lockedTimer.invalidate();
    endGraceTime();
    KDisplayManager().setLock(false);
    emit unlocked();
//     KNotification::event( QLatin1String("unlocked"));
}

bool KSldApp::startLockProcess(EstablishLock establishLock)
{
    QStringList args;
    if (establishLock == EstablishLock::Immediate) {
        args << "--immediateLock";
    }
    m_lockProcess->start(QStringLiteral(KSCREENLOCKER_GREET_BIN), args);
    // we wait one minute
    if (!m_lockProcess->waitForStarted(60000)) {
        m_lockProcess->kill();
        return false;
    }

    return true;
}

void KSldApp::showLockWindow()
{
    if (!m_lockWindow) {
        m_lockWindow = new LockWindow();
        connect(m_lockWindow, &LockWindow::userActivity, this,
            [this]() {
                if (isGraceTime()) {
                    unlock();
                }
            },
            Qt::QueuedConnection
        );
    }
    m_lockWindow->showLockWindow();
    XSync(QX11Info::display(), False);
}

void KSldApp::hideLockWindow()
{
    if (!m_lockWindow) {
        return;
    }
    m_lockWindow->hideLockWindow();
}

uint KSldApp::activeTime() const
{
    if (m_lockedTimer.isValid()) {
        return m_lockedTimer.elapsed();
    }
    return 0;
}

bool KSldApp::isGraceTime() const
{
    return m_inGraceTime;
}

void KSldApp::endGraceTime()
{
    m_graceTimer->stop();
    m_inGraceTime = false;
}

void KSldApp::unlock()
{
    if (!isGraceTime()) {
        return;
    }
    s_graceTimeKill = true;
    m_lockProcess->terminate();
}

void KSldApp::inhibit()
{
    ++m_inhibitCounter;
}

void KSldApp::uninhibit()
{
    --m_inhibitCounter;
}

} // namespace
#include "ksldapp.moc"
