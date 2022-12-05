/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2018 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "greeter.h"

#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QWaylandClientExtensionTemplate>

#include <qpa/qplatformwindow_p.h>

#include "shutdowndlg.h"

#include "logoutpromptadaptor.h"

#include <KWaylandExtras>
#include <KWindowSystem>
#include <LayerShellQt/Shell>

#include "qwayland-kde-lockscreen-overlay-v1.h"

using namespace Qt::StringLiterals;

class WaylandAboveLockscreen : public QWaylandClientExtensionTemplate<WaylandAboveLockscreen>, public QtWayland::kde_lockscreen_overlay_v1
{
public:
    WaylandAboveLockscreen()
        : QWaylandClientExtensionTemplate<WaylandAboveLockscreen>(1)
    {
        initialize();
    }

    void allowWindow(QWindow *window)
    {
        auto waylandWindow = window->nativeInterface<QNativeInterface::Private::QWaylandWindow>();
        if (!waylandWindow) {
            return;
        }

        wl_surface *surface = waylandWindow->surface();
        allow(surface);
    }
};

Greeter::Greeter(const KPackage::Package &package)
    : QObject()
    , m_package(package)
{
    new LogoutPromptAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/LogoutPrompt"), this);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.LogoutPrompt"));
}

Greeter::~Greeter()
{
    qDeleteAll(m_dialogs);
}

void Greeter::setupWaylandIntegration()
{
    if (!KWindowSystem::isPlatformWayland() || m_windowed) {
        return;
    }
    LayerShellQt::Shell::useLayerShell();
}

void Greeter::init()
{
    // If we're already shutting down we don't need another prompt,
    // just reply to the dbus message and exit
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(u"org.kde.Shutdown"_s)) {
        QApplication::quit();
        return;
    }

    setupWaylandIntegration();
    const auto screens = qApp->screens();
    for (QScreen *screen : screens) {
        adoptScreen(screen);
    }
    connect(qApp, &QGuiApplication::screenAdded, this, &Greeter::adoptScreen);
    m_running = true;
}

void Greeter::enableWindowed()
{
    m_windowed = true;
    promptAll();
}

void Greeter::adoptScreen(QScreen *screen)
{
    if (screen->geometry().isNull()) {
        return;
    }
    // TODO: last argument is the theme, maybe add command line option for it?
    KSMShutdownDlg *w = new KSMShutdownDlg(nullptr, m_shutdownType, screen);
    w->setWindowed(m_windowed);
    w->installEventFilter(this);
    m_dialogs << w;

    QObject::connect(screen, &QObject::destroyed, w, [w, this] {
        m_dialogs.removeOne(w);
        w->deleteLater();
    });
    connect(w, &KSMShutdownDlg::accepted, this, &Greeter::quit);
    connect(w, &KSMShutdownDlg::rejected, this, &Greeter::quit);

    w->setGeometry(screen->geometry());
    w->init(m_package);

    // lockscreen overlay
    if (KWindowSystem::isPlatformWayland()) {
        // get lockscreen state
        QDBusMessage request = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                              QStringLiteral("/ScreenSaver"),
                                                              QStringLiteral("org.freedesktop.ScreenSaver"),
                                                              QStringLiteral("GetActive"));
        const QDBusReply<bool> response = QDBusConnection::sessionBus().call(request);

        bool lockscreenShown = response.isValid() ? response.value() : false;

        if (lockscreenShown) {
            // allow window over lockscreen
            WaylandAboveLockscreen aboveLockscreen;
            if (aboveLockscreen.isInitialized()) {
                aboveLockscreen.allowWindow(w);
            }

            // put window over lockscreen
            KWaylandExtras::requestXdgActivationToken(w, 0, QStringLiteral("org.kde.ksmserver.greeter"));
            QObject::connect(KWaylandExtras::self(), &KWaylandExtras::xdgActivationTokenArrived, w, [w](int, const QString &token) {
                KWindowSystem::setCurrentXdgActivationToken(token);
                KWindowSystem::activateWindow(w);
            });
        }
    }
}

void Greeter::quit()
{
    for (auto window : qApp->topLevelWindows()) {
        window->close();
    }
}

bool Greeter::eventFilter(QObject *watched, QEvent *event)
{
    if (qobject_cast<KSMShutdownDlg *>(watched)) {
        if (event->type() == QEvent::MouseButtonPress) {
            // check that the position is on no window
            QMouseEvent *me = static_cast<QMouseEvent *>(event);
            if (std::any_of(m_dialogs.cbegin(), m_dialogs.cend(), [me](KSMShutdownDlg *dialog) {
                    return dialog->geometry().contains(me->globalPosition().toPoint());
                })) {
                return false;
            }
            // click outside, close
            quit();
        }
    }
    return false;
}

void Greeter::promptLogout()
{
    if (m_running) {
        return;
    }
    m_shutdownType = KWorkSpace::ShutdownTypeNone;
    init();
}

void Greeter::promptShutDown()
{
    if (m_running) {
        return;
    }
    m_shutdownType = KWorkSpace::ShutdownTypeHalt;
    init();
}

void Greeter::promptReboot()
{
    if (m_running) {
        return;
    }
    m_shutdownType = KWorkSpace::ShutdownTypeReboot;
    init();
}

void Greeter::promptAll()
{
    if (m_running) {
        return;
    }
    m_shutdownType = KWorkSpace::ShutdownTypeDefault;
    init();
}

#include "moc_greeter.cpp"
