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
#include "authenticator.h"

// KDE
#include <KAuthorized>
#include <KCrash>
#include <KUser>
#include <KWindowSystem>
#include <Solid/PowerManagement>
//Plasma
#include <Plasma/Package>
#include <Plasma/PackageStructure>
#include <Plasma/PluginLoader>
// Qt
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <qscreen.h>

#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
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
UnlockApp::UnlockApp(int &argc, char **argv)
    : QGuiApplication(argc, argv)
    , m_resetRequestIgnoreTimer(new QTimer(this))
    , m_delayedLockTimer(0)
    , m_testing(false)
    , m_ignoreRequests(false)
    , m_immediateLock(false)
    , m_authenticator(new Authenticator(this))
    , m_graceTime(0)
    , m_noLock(false)
{
    connect(m_authenticator, &Authenticator::succeeded, this, &QCoreApplication::quit);
    initialize();
    connect(this, SIGNAL(screenAdded(QScreen*)), SLOT(desktopResized()));
}

UnlockApp::~UnlockApp()
{
    qDeleteAll(m_views);
}

void UnlockApp::initialize()
{
    const char *uri = "org.kde.kscreenlocker";
    //FIXME
//     qmlRegisterType<GreeterItem>(uri, 1, 0, "GreeterItem");
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
    Plasma::Package package = Plasma::PluginLoader::self()->loadPackage("Plasma/LookAndFeel");
    KConfigGroup cg(KSharedConfig::openConfig("kdeglobals"), "KDE");
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    if (!packageName.isEmpty()) {
        package.setPath(packageName);
    }
    if (!KScreenSaverSettings::theme().isEmpty()) {
        package.setPath(KScreenSaverSettings::theme());
    }

    m_mainQmlPath = QUrl::fromLocalFile(package.filePath("lockscreenmainscript"));

    installEventFilter(this);
}

void UnlockApp::viewStatusChanged(const QQuickView::Status &status)
{
    // on error, if we did not load the default qml, try to do so now.
    if (status == QQuickView::Error &&
        m_package.metadata().pluginName() != QLatin1String(DEFAULT_MAIN_PACKAGE)) {
        if (QQuickView *view = qobject_cast<QQuickView *>(sender())) {
            m_package.setPath(QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                     QStringLiteral("ksmserver/screenlocker/") + QString::fromLatin1(DEFAULT_MAIN_PACKAGE)));

            m_mainQmlPath = QUrl("qrc:/fallbacktheme/LockScreen.qml");
            view->setSource(m_mainQmlPath);
        }
    }
}

void UnlockApp::desktopResized()
{
    const int nScreens = screens().count();
    // remove useless views and savers
    while (m_views.count() > nScreens) {
        m_views.takeLast()->deleteLater();
    }

    // extend views and savers to current demand
    const bool canLogout = KAuthorized::authorizeKAction(QStringLiteral("logout")) && KAuthorized::authorize(QStringLiteral("logout"));
    const QSet<Solid::PowerManagement::SleepState> spdMethods = Solid::PowerManagement::supportedSleepStates();
    for (int i = m_views.count(); i < nScreens; ++i) {
        connect(QGuiApplication::screens()[i], SIGNAL(destroyed(QObject*)), SLOT(desktopResized()));
        // create the view
        QQuickView *view = new QQuickView();
        connect(view, SIGNAL(statusChanged(QQuickView::Status)),
                this, SLOT(viewStatusChanged(QQuickView::Status)));

        if (!m_testing) {
            view->setFlags(Qt::X11BypassWindowManagerHint);
        }

        // engine stuff
        QQmlContext* context = view->engine()->rootContext();
        const KUser user;
        const QString fullName = user.property(KUser::FullName).toString();

        context->setContextProperty(QStringLiteral("kscreenlocker_userName"), fullName.isEmpty() ? user.loginName() : fullName);
        context->setContextProperty(QStringLiteral("kscreenlocker_userImage"), user.faceIconPath());
        context->setContextProperty(QStringLiteral("authenticator"), m_authenticator);

        view->setSource(m_mainQmlPath);
        view->setResizeMode(QQuickView::SizeRootObjectToView);

        QQmlProperty lockProperty(view->rootObject(), QStringLiteral("locked"));
        lockProperty.write(m_immediateLock || (!m_noLock && !m_delayedLockTimer));

        QQmlProperty sleepProperty(view->rootObject(), QStringLiteral("suspendToRamSupported"));
        sleepProperty.write(spdMethods.contains(Solid::PowerManagement::SuspendState));
        if (spdMethods.contains(Solid::PowerManagement::SuspendState) &&
            view->rootObject()->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("suspendToRam()").constData()) != -1) {
            connect(view->rootObject(), SIGNAL(suspendToRam()), SLOT(suspendToRam()));
        }

        QQmlProperty hibernateProperty(view->rootObject(), QStringLiteral("suspendToDiskSupported"));
        hibernateProperty.write(spdMethods.contains(Solid::PowerManagement::HibernateState));
        if (spdMethods.contains(Solid::PowerManagement::HibernateState) &&
            view->rootObject()->metaObject()->indexOfSignal(QMetaObject::normalizedSignature("suspendToDisk()").constData()) != -1) {
            connect(view->rootObject(), SIGNAL(suspendToDisk()), SLOT(suspendToDisk()));
        }

        m_views << view;
    }

    // update geometry of all views and savers
    for (int i = 0; i < nScreens; ++i) {
        QQuickView *view = m_views.at(i);

        view->setGeometry(QGuiApplication::screens()[i]->geometry());
        view->show();
        view->raise();
    }
    // random state update, actually rather required on init only
    QMetaObject::invokeMethod(this, "getFocus", Qt::QueuedConnection);
    // getFocus on the next event cycle does not work as expected for multiple views
    // if there's no screensaver, hiding it won't happen and thus not trigger getFocus either
    // so we call it again in a few miliseconds - the value is nearly random but "must cross some event cycles"
    // while 150ms worked for me, 250ms gets us a bit more padding without being notable to a human user
    if (nScreens > 1) {
        QTimer::singleShot(250, this, SLOT(getFocus()));
    }
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

void UnlockApp::lockImmediately()
{
    setImmediateLock(true);
    setLockedPropertyOnViews();
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

    if (event->type() == QEvent::KeyPress) { // react if saver is visible
        shareEvent(event, qobject_cast<QQuickView*>(obj));
        return false; // we don't care
    } else if (event->type() == QEvent::KeyRelease) { // conditionally reshow the saver
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() != Qt::Key_Escape) {
            shareEvent(event, qobject_cast<QQuickView*>(obj));
            return false; // irrelevant
        }
        return true; // don't pass
    }

    return false;
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
                QCoreApplication::sendEvent(view, e);
                e->setAccepted(accepted);
            }
        }
        installEventFilter(this);
    }
}

void UnlockApp::setGraceTime(int milliseconds)
{
    m_graceTime = milliseconds;
    if (milliseconds < 0 || m_delayedLockTimer || m_noLock || m_immediateLock) {
        return;
    }
    m_delayedLockTimer = new QTimer(this);
    m_delayedLockTimer->setSingleShot(true);
    connect(m_delayedLockTimer, &QTimer::timeout, this, &UnlockApp::setLockedPropertyOnViews);
    m_delayedLockTimer->start(m_graceTime);
}

void UnlockApp::setNoLock(bool noLock)
{
    m_noLock = noLock;
}

} // namespace

#include "greeterapp.moc"
