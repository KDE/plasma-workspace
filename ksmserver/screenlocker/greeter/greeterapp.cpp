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
#include "noaccessnetworkaccessmanagerfactory.h"

// KDE
#include <KAuthorized>
#include <KCrash>
#include <kdeclarative/kdeclarative.h>
#include <KDeclarative/KQuickAddons/QuickViewSharedEngine>
#include <KUser>
#include <KWindowSystem>
#include <Solid/PowerManagement>
//Plasma
#include <KPackage/Package>
#include <KPackage/PackageStructure>
#include <KPackage/PackageLoader>
// KWayland
#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/event_queue.h>
#include <KWayland/Client/registry.h>
// Qt
#include <QAbstractNativeEventFilter>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include <qscreen.h>
#include <QThread>

#include <QQuickView>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>

#include <QX11Info>
// Wayland
#include <wayland-client.h>
#include <wayland-ksld-client-protocol.h>
// X11
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <fixx11h.h>
//
#include <xcb/xcb.h>

// this is usable to fake a "screensaver" installation for testing
// *must* be "0" for every public commit!
#define TEST_SCREENSAVER 0

namespace ScreenLocker
{

class FocusOutEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long int *result) override {
        Q_UNUSED(result)
        if (qstrcmp(eventType, "xcb_generic_event_t") != 0) {
            return false;
        }
        xcb_generic_event_t *event = reinterpret_cast<xcb_generic_event_t*>(message);
        if ((event->response_type & ~0x80) == XCB_FOCUS_OUT) {
            return true;
        }
        return false;
    }
};

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
    connect(this, &UnlockApp::screenAdded, this, &UnlockApp::desktopResized);
    if (QX11Info::isPlatformX11()) {
        installNativeEventFilter(new FocusOutEventFilter);
    }
}

UnlockApp::~UnlockApp()
{
    qDeleteAll(m_views);

    if (m_ksldInterface) {
        org_kde_ksld_destroy(m_ksldInterface);
    }
    if (m_ksldRegistry) {
        delete m_ksldRegistry;
    }
    if (m_ksldConnection) {
        m_ksldConnection->deleteLater();
        m_ksldConnectionThread->quit();
        m_ksldConnectionThread->wait();
    }
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
    connect(m_resetRequestIgnoreTimer, &QTimer::timeout, this, &UnlockApp::resetRequestIgnore);

    // disable DrKonqi as the crash dialog blocks the restart of the locker
    KCrash::setDrKonqiEnabled(false);

    KScreenSaverSettings::self()->load();
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage("Plasma/LookAndFeel");
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

void UnlockApp::viewStatusChanged(const QQmlComponent::Status &status)
{
    auto *view = qobject_cast<KQuickAddons::QuickViewSharedEngine *>(sender());
    if (!view) {
        return;
    }

    const QUrl fallbackUrl("qrc:/fallbacktheme/LockScreen.qml");

    // on error, load the fallback lockscreen to not lock the user out of the system
    if (status == QQmlComponent::Error && view && view->source() != fallbackUrl) {
        m_mainQmlPath = fallbackUrl;
        view->setSource(m_mainQmlPath);
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
        auto *view = new KQuickAddons::QuickViewSharedEngine();
        connect(view, &KQuickAddons::QuickViewSharedEngine::statusChanged, this, &UnlockApp::viewStatusChanged);
        view->setColor(Qt::black);

        // first create KDeclarative, to be sure that it created a KIO Network Factory
        KDeclarative::KDeclarative declarative;
        declarative.setDeclarativeEngine(view->engine());
        declarative.setupBindings();
        // overwrite the factory set by kdeclarative
        auto oldFactory = view->engine()->networkAccessManagerFactory();
        view->engine()->setNetworkAccessManagerFactory(nullptr);
        delete oldFactory;
        view->engine()->setNetworkAccessManagerFactory(new NoAccessNetworkAccessManagerFactory);

        if (!m_testing) {
            if (QX11Info::isPlatformX11()) {
                view->setFlags(Qt::X11BypassWindowManagerHint);
            } else {
                view->setFlags(Qt::FramelessWindowHint);
            }
        }

        if (m_ksldInterface) {
            view->create();
            org_kde_ksld_x11window(m_ksldInterface, view->winId());
            wl_display_flush(m_ksldConnection->display());
        }

        // engine stuff
        QQmlContext* context = view->engine()->rootContext();
        const KUser user;
        const QString fullName = user.property(KUser::FullName).toString();

        context->setContextProperty(QStringLiteral("kscreenlocker_userName"), fullName.isEmpty() ? user.loginName() : fullName);
        context->setContextProperty(QStringLiteral("kscreenlocker_userImage"), user.faceIconPath());
        context->setContextProperty(QStringLiteral("authenticator"), m_authenticator);
        context->setContextProperty(QStringLiteral("backgroundPath"), KScreenSaverSettings::themeBackground());
        context->setContextProperty(QStringLiteral("org_kde_plasma_screenlocker_greeter_interfaceVersion"), 1);

        view->setSource(m_mainQmlPath);
        view->setResizeMode(KQuickAddons::QuickViewSharedEngine::SizeRootObjectToView);

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

        // verify that the engine's controller didn't change
        Q_ASSERT(dynamic_cast<NoAccessNetworkAccessManagerFactory*>(view->engine()->networkAccessManagerFactory()));

        m_views << view;
    }

    // update geometry of all views and savers
    for (int i = 0; i < nScreens; ++i) {
        auto *view = m_views.at(i);

        view->setGeometry(QGuiApplication::screens()[i]->geometry());
        if (m_testing) {
            view->show();
        } else {
            view->showFullScreen();
        }
        view->raise();
        connect(view, &QQuickWindow::frameSwapped, this, &UnlockApp::markViewsAsVisible, Qt::QueuedConnection);
    }
}

void UnlockApp::markViewsAsVisible()
{
    auto *view = qobject_cast<KQuickAddons::QuickViewSharedEngine *>(sender());
    disconnect(view, &QQuickWindow::frameSwapped, this, &UnlockApp::markViewsAsVisible);
    QQmlProperty showProperty(view->rootObject(), QStringLiteral("viewVisible"));
    showProperty.write(true);
    // random state update, actually rather required on init only
    QMetaObject::invokeMethod(this, "getFocus", Qt::QueuedConnection);
}

void UnlockApp::getFocus()
{
    if (m_views.isEmpty()) {
        return;
    }
    QWindow *w = 0;
    // this loop is required to make the qml/graphicsscene properly handle the shared keyboard input
    // ie. "type something into the box of every greeter"
    foreach (KQuickAddons::QuickViewSharedEngine *view, m_views) {
        view->requestActivate();
        if (!m_testing) {
            view->setKeyboardGrabEnabled(true); // TODO - check whether this still works in master!
        }
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
        foreach (KQuickAddons::QuickViewSharedEngine *view, m_views) {
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
    if (!m_testing) {
        w->setKeyboardGrabEnabled(true); // TODO - check whether this still works in master!
    }
    w->requestActivate();
//     w->setFocus(Qt::OtherFocusReason); // FIXME
}

void UnlockApp::setLockedPropertyOnViews()
{
    delete m_delayedLockTimer;
    m_delayedLockTimer = 0;

    foreach (KQuickAddons::QuickViewSharedEngine *view, m_views) {
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
        foreach (KQuickAddons::QuickViewSharedEngine * view, m_views) {
            view->setFlags(view->flags() & ~Qt::X11BypassWindowManagerHint);
        }
    } else {
        foreach (KQuickAddons::QuickViewSharedEngine * view, m_views) {
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
        KQuickAddons::QuickViewSharedEngine *view(0);
        foreach (KQuickAddons::QuickViewSharedEngine *v, m_views) {
            if (v == obj) {
                view = v;
                break;
            }
        }
        if (view && view->winId() && QX11Info::isPlatformX11()) {
            // showing greeter view window, set property
            static Atom tag = XInternAtom(QX11Info::display(), "_KDE_SCREEN_LOCKER", False);
            XChangeProperty(QX11Info::display(), view->winId(), tag, tag, 32, PropModeReplace, 0, 0);
        }
        // no further processing
        return false;
    }

    if (event->type() == QEvent::KeyPress) { // react if saver is visible
        shareEvent(event, qobject_cast<KQuickAddons::QuickViewSharedEngine*>(obj));
        return false; // we don't care
    } else if (event->type() == QEvent::KeyRelease) { // conditionally reshow the saver
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() != Qt::Key_Escape) {
            shareEvent(event, qobject_cast<KQuickAddons::QuickViewSharedEngine*>(obj));
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

void UnlockApp::shareEvent(QEvent *e, KQuickAddons::QuickViewSharedEngine *from)
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
        foreach (KQuickAddons::QuickViewSharedEngine *view, m_views) {
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


static void osdProgress(void *data, org_kde_ksld *org_kde_ksld, const char *icon, int32_t percent, const char *text)
{
    Q_UNUSED(org_kde_ksld)
    reinterpret_cast<UnlockApp*>(data)->osdProgress(QString::fromUtf8(icon), percent, QString::fromUtf8(text));
}

static void osdText(void *data, org_kde_ksld *org_kde_ksld, const char *icon, const char *text)
{
    Q_UNUSED(org_kde_ksld)
    reinterpret_cast<UnlockApp*>(data)->osdText(QString::fromUtf8(icon), QString::fromUtf8(text));
}

static const struct org_kde_ksld_listener s_listener {
    osdProgress,
    osdText
};

void UnlockApp::setKsldSocket(int socket)
{
    using namespace KWayland::Client;
    m_ksldConnection = new ConnectionThread;
    m_ksldConnection->setSocketFd(socket);

    m_ksldRegistry = new Registry();
    EventQueue *queue = new EventQueue(m_ksldRegistry);

    connect(m_ksldRegistry, &Registry::interfaceAnnounced, this,
        [this, queue] (QByteArray interface, quint32 name, quint32 version) {
            if (interface != QByteArrayLiteral("org_kde_ksld")) {
                return;
            }
            m_ksldInterface = reinterpret_cast<org_kde_ksld*>(wl_registry_bind(*m_ksldRegistry, name, &org_kde_ksld_interface, version));
            queue->addProxy(m_ksldInterface);
            if (version >= 2) {
                org_kde_ksld_add_listener(m_ksldInterface, &s_listener, this);
            }
            for (auto v : m_views) {
                org_kde_ksld_x11window(m_ksldInterface, v->winId());
                wl_display_flush(m_ksldConnection->display());
            }
        }
    );

    connect(m_ksldConnection, &ConnectionThread::connected, this,
        [this, queue] {
            m_ksldRegistry->create(m_ksldConnection);
            queue->setup(m_ksldConnection);
            m_ksldRegistry->setEventQueue(queue);
            m_ksldRegistry->setup();
            wl_display_flush(m_ksldConnection->display());
        }, Qt::QueuedConnection);

    m_ksldConnectionThread = new QThread(this);
    m_ksldConnection->moveToThread(m_ksldConnectionThread);
    m_ksldConnectionThread->start();
    m_ksldConnection->initConnection();
}

void UnlockApp::osdProgress(const QString &icon, int percent, const QString &additionalText)
{
    for (auto v : m_views) {
        auto osd = v->rootObject()->findChild<QQuickItem*>(QStringLiteral("onScreenDisplay"));
        if (!osd) {
            continue;
        }
        osd->setProperty("osdValue", percent);
        osd->setProperty("osdAdditionalText", additionalText);
        osd->setProperty("showingProgress", true);
        osd->setProperty("icon", icon);
        QMetaObject::invokeMethod(osd, "show");
    }
}

void UnlockApp::osdText(const QString &icon, const QString &additionalText)
{
    for (auto v : m_views) {
        auto osd = v->rootObject()->findChild<QQuickItem*>(QStringLiteral("onScreenDisplay"));
        if (!osd) {
            continue;
        }
        osd->setProperty("showingProgress", false);
        osd->setProperty("osdValue", additionalText);
        osd->setProperty("icon", icon);
        QMetaObject::invokeMethod(osd, "show");
    }
}

} // namespace

