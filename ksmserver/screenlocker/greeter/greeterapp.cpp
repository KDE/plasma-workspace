/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2004 Chris Howells <howells@kde.org>
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
#include "greeterapp.h"
#include "kscreensaversettings.h"
#include "sessions.h"
#include "screensaverwindow.h"

// workspace
#include <kworkspace.h>
// KDE
#include <KAuthorized>
#include <KCrash>
#include <KDebug>
#include <KStandardDirs>
#include <KUser>
#include <KWindowSystem>
#include <Solid/PowerManagement>
#include <kdeclarative/kdeclarative.h>
//Plasma
#include <Plasma/Package>
#include <Plasma/PackageStructure>
#include <Plasma/PluginLoader>
// Qt
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <QDesktopWidget>

#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlProperty>

#include <QX11Info>
// X11
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <fixx11h.h>

// this is usable to fake a "screensaver" installation for testing
// *must* be "0" for every public commit!
#define TEST_SCREENSAVER 0

namespace ScreenLocker
{

static const char *DEFAULT_MAIN_PACKAGE = "org.kde.passworddialog";

// App
UnlockApp::UnlockApp()
    : KApplication()
    , m_resetRequestIgnoreTimer(new QTimer(this))
    , m_delayedLockTimer(0)
    , m_package(Plasma::PluginLoader::self()->loadPackage("Plasma/Applet"))
    , m_testing(false)
    , m_capsLocked(false)
    , m_ignoreRequests(false)
    , m_showScreenSaver(false)
    , m_immediateLock(false)
    , m_runtimeInitialized(false)
{
    initialize();
    connect(desktop(), SIGNAL(resized(int)), SLOT(desktopResized()));
    connect(desktop(), SIGNAL(screenCountChanged(int)), SLOT(desktopResized()));
}

UnlockApp::~UnlockApp()
{
    qDeleteAll(m_views);
    qDeleteAll(m_screensaverWindows);
}

void UnlockApp::initialize()
{
    const char *uri = "org.kde.kscreenlocker";
    //FIXME
//     qmlRegisterType<GreeterItem>(uri, 1, 0, "GreeterItem");
//     qmlRegisterType<KeyboardItem>(uri, 1, 0, "KeyboardItem");
    qmlRegisterType<SessionSwitching>(uri, 1, 0, "Sessions");
    qmlRegisterType<QAbstractItemModel>();

    // set up the request ignore timeout, so that multiple requests to sleep/suspend/shutdown
    // are not processed in quick (and confusing) succession)
    m_resetRequestIgnoreTimer->setSingleShot(true);
    m_resetRequestIgnoreTimer->setInterval(2000);
    connect(m_resetRequestIgnoreTimer, SIGNAL(timeout()), this, SLOT(resetRequestIgnore()));

    // disable DrKonqi as the crash dialog blocks the restart of the locker
    KCrash::setDrKonqiEnabled(false);

    KScreenSaverSettings::self()->readConfig();
#if TEST_SCREENSAVER
    m_showScreenSaver = true;
#else
    m_showScreenSaver = KScreenSaverSettings::legacySaverEnabled();
#endif

    m_package.setPath(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("ksmserver/screenlocker/") + KScreenSaverSettings::greeterQML(), QStandardPaths::LocateDirectory));

    m_mainQmlPath = m_package.filePath("mainscript");

    if (m_mainQmlPath.isEmpty()) {
        m_package.setPath(KStandardDirs::locate("data", QStringLiteral("ksmserver/screenlocker/") + QString::fromLatin1(DEFAULT_MAIN_PACKAGE)));
        m_mainQmlPath = m_package.filePath("mainscript");
    }

    installEventFilter(this);
}

void UnlockApp::viewStatusChanged(const QQuickView::Status &status)
{
    // on error, if we did not load the default qml, try to do so now.
    if (status == QQuickView::Error &&
        m_package.metadata().pluginName() != QLatin1String(DEFAULT_MAIN_PACKAGE)) {
        if (QQuickView *view = qobject_cast<QQuickView *>(sender())) {
            m_package.setPath(KStandardDirs::locate("data", QStringLiteral("ksmserver/screenlocker/") + QString::fromLatin1(DEFAULT_MAIN_PACKAGE)));

            m_mainQmlPath = m_package.filePath("mainscript");
            view->setSource(QUrl::fromLocalFile(m_mainQmlPath));
        }
    }
}

void UnlockApp::desktopResized()
{
    const int nScreens = desktop()->screenCount();
    // remove useless views and savers
    while (m_views.count() > nScreens) {
        m_views.takeLast()->deleteLater();
    }
    while (m_screensaverWindows.count() > nScreens) {
        m_screensaverWindows.takeLast()->deleteLater();
    }

    Q_ASSERT((!m_showScreenSaver || m_views.count() == m_screensaverWindows.count()));

    // extend views and savers to current demand
    const bool canLogout = KAuthorized::authorizeKAction(QStringLiteral("logout")) && KAuthorized::authorize(QStringLiteral("logout"));
    const QSet<Solid::PowerManagement::SleepState> spdMethods = Solid::PowerManagement::supportedSleepStates();
    for (int i = m_views.count(); i < nScreens; ++i) {
        // create the view
        QQuickView *view = new QQuickView();
        connect(view, SIGNAL(statusChanged(QQuickView::Status)),
                this, SLOT(viewStatusChanged(QQuickView::Status)));

        if (!m_testing) {
            view->setFlags(Qt::X11BypassWindowManagerHint);
        }

        // engine stuff
        KDeclarative::KDeclarative kdeclarative;
        kdeclarative.setDeclarativeEngine(view->engine());
        kdeclarative.initialize();
        kdeclarative.setupBindings();
        QQmlContext* context = view->engine()->rootContext();
        const KUser user;
        const QString fullName = user.property(KUser::FullName).toString();

        context->setContextProperty(QStringLiteral("kscreenlocker_userName"), fullName.isEmpty() ? user.loginName() : fullName);

        view->setSource(QUrl::fromLocalFile(m_mainQmlPath));
        view->setResizeMode(QQuickView::SizeRootObjectToView);

        connect(view->rootObject(), SIGNAL(unlockRequested()), SLOT(quit()));

        QQmlProperty lockProperty(view->rootObject(), QStringLiteral("locked"));
        if (m_immediateLock) {
            lockProperty.write(true);
        } else if (KScreenSaverSettings::lock()) {
            if (KScreenSaverSettings::lockGrace() < 1) {
                lockProperty.write(true);
            } else if (m_runtimeInitialized) {
                // if we have new views and we are waiting on the
                // delayed lock timer still, we don't want to show
                // the lock UI just yet
                lockProperty.write(!m_delayedLockTimer);
            } else {
                if (!m_delayedLockTimer) {
                    m_delayedLockTimer = new QTimer(this);
                    m_delayedLockTimer->setSingleShot(true);
                    connect(m_delayedLockTimer, SIGNAL(timeout()), this, SLOT(setLockedPropertyOnViews()));
                }
                m_delayedLockTimer->start(KScreenSaverSettings::lockGrace());
            }
        } else {
            lockProperty.write(false);
        }

        QQmlProperty sleepProperty(view->rootObject(), QStringLiteral("suspendToRamSupported"));
        sleepProperty.write(spdMethods.contains(Solid::PowerManagement::SuspendState));
        if (spdMethods.contains(Solid::PowerManagement::SuspendState) &&
            view->rootObject()->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("suspendToRam()").constData()) != -1) {
            connect(view->rootObject(), SIGNAL(suspendToRam()), SLOT(suspendToRam()));
        }

        QQmlProperty hibernateProperty(view->rootObject(), QStringLiteral("suspendToDiskSupported"));
        hibernateProperty.write(spdMethods.contains(Solid::PowerManagement::SuspendState));
        if (spdMethods.contains(Solid::PowerManagement::SuspendState) &&
            view->rootObject()->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("suspendToDisk()").constData()) != -1) {
            connect(view->rootObject(), SIGNAL(suspendToDisk()), SLOT(suspendToDisk()));
        }

        QQmlProperty shutdownProperty(view->rootObject(), QStringLiteral("shutdownSupported"));
        shutdownProperty.write(canLogout);
        if (canLogout &&
            view->rootObject()->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("shutdown()").constData()) != -1) {
            connect(view->rootObject(), SIGNAL(shutdown()), SLOT(shutdown()));
        }

        m_views << view;

        if (m_showScreenSaver) {
            ScreenSaverWindow *screensaverWindow = new ScreenSaverWindow;
            screensaverWindow->setWindowFlags(Qt::X11BypassWindowManagerHint);
            m_screensaverWindows << screensaverWindow;
        }
    }

    m_runtimeInitialized = true;

    // update geometry of all views and savers
    for (int i = 0; i < nScreens; ++i) {
        QQuickView *view = m_views.at(i);

        view->setGeometry(desktop()->screenGeometry(i));
        view->show();
        view->raise();

        if (m_showScreenSaver) {
            ScreenSaverWindow *screensaverWindow = m_screensaverWindows.at(i);
            screensaverWindow->setGeometry(view->geometry());

#if TEST_SCREENSAVER
            screensaverWindow->setAutoFillBackground(true);
#else
            const QPixmap backgroundPix = QPixmap::fromImage(view->grabWindow());
            screensaverWindow->setBackground(backgroundPix);
#endif
            screensaverWindow->show();
            screensaverWindow->activateWindow();
            connect(screensaverWindow, SIGNAL(hidden()), this, SLOT(getFocus()));
        }
    }
    // random state update, actually rather required on init only
    QMetaObject::invokeMethod(this, "getFocus", Qt::QueuedConnection);
    // getFocus on the next event cycle does not work as expected for multiple views
    // if there's no screensaver, hiding it won't happen and thus not trigger getFocus either
    // so we call it again in a few miliseconds - the value is nearly random but "must cross some event cycles"
    // while 150ms worked for me, 250ms gets us a bit more padding without being notable to a human user
    if (nScreens > 1 && m_screensaverWindows.isEmpty()) {
        QTimer::singleShot(250, this, SLOT(getFocus()));
    }
    capsLocked();
}

void UnlockApp::getFocus()
{
    if (m_views.isEmpty()) {
        return;
    }
    QWindow *w = 0;
    // this loop is required to make the qml/graphicsscene properly handle the shared keyboard input
    // ie. "type something into the box of every greeter"
    foreach (QQuickView *view, m_views) {
        view->requestActivate();
        view->setKeyboardGrabEnabled(true); // TODO - check whether this still works in master!
//         w->setFocus(Qt::OtherFocusReason); // FIXME
    }
    // determine which window should actually be active and have the real input focus/grab
    // FIXME - QWidget::underMouse()
//     foreach (QQuickView *view, m_views) {
//         if (view->underMouse()) {
//             w = view;
//             break;
//         }
//     }
    if (!w) { // try harder
        foreach (QQuickView *view, m_views) {
            if (view->geometry().contains(QCursor::pos())) {
                w = view;
                break;
            }
        }
    }
    if (!w) { // fallback solution
        w = m_views.first();
    }
    // activate window and grab input to be sure it really ends up there.
    // focus setting is still required for proper internal QWidget state (and eg. visual reflection)
    w->setKeyboardGrabEnabled(true); // TODO - check whether this still works in master!
    w->requestActivate();
//     w->setFocus(Qt::OtherFocusReason); // FIXME
}

void UnlockApp::setLockedPropertyOnViews()
{
    delete m_delayedLockTimer;
    m_delayedLockTimer = 0;

    foreach (QQuickView *view, m_views) {
        QQmlProperty lockProperty(view->rootObject(), QStringLiteral("locked"));
        lockProperty.write(true);
    }
}

void UnlockApp::resetRequestIgnore()
{
    m_ignoreRequests = false;
}

void UnlockApp::suspendToRam()
{
    if (m_ignoreRequests) {
        return;
    }

    m_ignoreRequests = true;
    m_resetRequestIgnoreTimer->start();

    Solid::PowerManagement::requestSleep(Solid::PowerManagement::SuspendState, 0, 0);

}

void UnlockApp::suspendToDisk()
{
    if (m_ignoreRequests) {
        return;
    }

    m_ignoreRequests = true;
    m_resetRequestIgnoreTimer->start();

    Solid::PowerManagement::requestSleep(Solid::PowerManagement::HibernateState, 0, 0);
}

void UnlockApp::shutdown()
{
    if (m_ignoreRequests) {
        return;
    }

    m_ignoreRequests = true;
    m_resetRequestIgnoreTimer->start();

    const KWorkSpace::ShutdownConfirm confirm = KWorkSpace::ShutdownConfirmNo;
    const KWorkSpace::ShutdownType type = KWorkSpace::ShutdownTypeHalt;

    KWorkSpace::requestShutDown(confirm, type);
}

void UnlockApp::setTesting(bool enable)
{
    m_testing  = enable;
    if (m_views.isEmpty()) {
        return;
    }
    if (enable) {
        // remove bypass window manager hint
        foreach (QQuickView * view, m_views) {
            view->setFlags(view->flags() & ~Qt::X11BypassWindowManagerHint);
        }
    } else {
        foreach (QQuickView * view, m_views) {
            view->setFlags(view->flags() | Qt::X11BypassWindowManagerHint);
        }
    }
}

void UnlockApp::setImmediateLock(bool immediate)
{
    m_immediateLock = immediate;
}

bool UnlockApp::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != this && event->type() == QEvent::Show) {
        QQuickView *view(0);
        foreach (QQuickView *v, m_views) {
            if (v == obj) {
                view = v;
                break;
            }
        }
        if (view && view->winId()) {
            // showing greeter view window, set property
            static Atom tag = XInternAtom(QX11Info::display(), "_KDE_SCREEN_LOCKER", False);
            XChangeProperty(QX11Info::display(), view->winId(), tag, tag, 32, PropModeReplace, 0, 0);
        }
        // no further processing
        return false;
    }

    static bool ignoreNextEscape = false;
    if (event->type() == QEvent::KeyPress) { // react if saver is visible
        bool saverVisible = !m_screensaverWindows.isEmpty();
        foreach (ScreenSaverWindow *screensaverWindow, m_screensaverWindows) {
            if (!screensaverWindow->isVisible()) {
                saverVisible = false;
                break;
            }
        }
        if (!saverVisible) {
            shareEvent(event, qobject_cast<QQuickView*>(obj));
            return false; // we don't care
        }
        ignoreNextEscape = bool(static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape);
        capsLocked();
        foreach (ScreenSaverWindow *screensaverWindow, m_screensaverWindows) {
            screensaverWindow->hide();
        }
        getFocus();
        return true; // do not pass the key
    } else if (event->type() == QEvent::KeyRelease) { // conditionally reshow the saver
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_CapsLock) {
            capsLocked();
            return false;
        }
        if (ke->key() != Qt::Key_Escape) {
            shareEvent(event, qobject_cast<QQuickView*>(obj));
            return false; // irrelevant
        }
        if (ignoreNextEscape) {
            ignoreNextEscape = false;
            return true; // it's Qt::Key_Escape;
        }
        bool saverVisible = true;
        foreach (ScreenSaverWindow *screensaverWindow, m_screensaverWindows) {
            if (!screensaverWindow->isVisible()) {
                saverVisible = false;
                break;
            }
        }
        if (saverVisible) {
            return false; // we don't care
        }
        foreach (ScreenSaverWindow *screensaverWindow, m_screensaverWindows) {
            screensaverWindow->show();
        }
        return true; // don't pass
    } else if (event->type() == QEvent::GraphicsSceneMousePress) {

        //FIXME FIXME
//         QGraphicsSceneMouseEvent *me = static_cast<QGraphicsSceneMouseEvent *>(event);
//
//         foreach (QQuickView *view, m_views) {
//             if (view->geometry().contains(me->screenPos())) {
//                 view->activateWindow();
//                 view->grabKeyboard();
//                 break;
//             }
//         }
    }

    return false;
}

void UnlockApp::capsLocked()
{
    unsigned int lmask;
    Window dummy1, dummy2;
    int dummy3, dummy4, dummy5, dummy6;
    XQueryPointer(QX11Info::display(), DefaultRootWindow( QX11Info::display() ), &dummy1, &dummy2, &dummy3, &dummy4, &dummy5, &dummy6, &lmask);
    const bool before = m_capsLocked;
    m_capsLocked = lmask & LockMask;
    if (before != m_capsLocked) {
        foreach (QQuickView *view, m_views) {
            view->rootObject()->setProperty("capsLockOn", m_capsLocked);
        }
    }
}

/*
 * This function forwards an event from one greeter window to all others
 * It's used to have the keyboard operate on all greeter windows (on every screen)
 * at once so that the user gets visual feedback on the screen he's looking at -
 * even if the focus is actually on a powered off screen.
 */

void UnlockApp::shareEvent(QEvent *e, QQuickView *from)
{
    // from can be NULL any time (because the parameter is passed as qobject_cast)
    // m_views.contains(from) is atm. supposed to be true but required if any further
    // QQuickView are added (which are not part of m_views)
    // this makes "from" an optimization (nullptr check aversion)
    if (from && m_views.contains(from)) {
        // NOTICE any recursion in the event sharing will prevent authentication on multiscreen setups!
        // Any change in regarded event processing shall be tested thoroughly!
        removeEventFilter(this); // prevent recursion!
        const bool accepted = e->isAccepted(); // store state
        foreach (QQuickView *view, m_views) {
            if (view != from) {
                QApplication::sendEvent(view, e);
                e->setAccepted(accepted);
            }
        }
        installEventFilter(this);
    }
}

} // namespace

#include "greeterapp.moc"
