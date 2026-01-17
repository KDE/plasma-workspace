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

#include "debug.h"
#include "logoutpromptadaptor.h"
#include "shutdowndlg.h"

#include <algorithm>

using namespace std::chrono_literals;
using namespace Qt::StringLiterals;

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

void Greeter::init()
{
    // If we're already shutting down we don't need another prompt,
    // just reply to the dbus message and exit
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(u"org.kde.Shutdown"_s)) {
        QApplication::quit();
        return;
    }

    if (!m_windowed) {
        // Quit if we lost focus or failed to gain it after 1 second
        auto *quitTimer = new QTimer(this);
        QObject::connect(quitTimer, &QTimer::timeout, this, [this] {
            qCWarning(LOGOUT_GREETER) << "Failed to get focus after 1 second, quitting";
            quit();
        });
        quitTimer->setInterval(1s);
        quitTimer->setSingleShot(true);
        quitTimer->start();
        QObject::connect(qApp, &QGuiApplication::applicationStateChanged, this, [this, quitTimer = QPointer(quitTimer)](Qt::ApplicationState state) {
            switch (state) {
            case Qt::ApplicationActive:
                if (!quitTimer.isNull()) {
                    quitTimer->stop();
                    quitTimer->deleteLater();
                }
                break;
            case Qt::ApplicationInactive:
                qCWarning(LOGOUT_GREETER) << "Lost focus, quitting";
                quit();
                break;
            default:
                break;
            }
        });
    }

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
    auto *w = new KSMShutdownDlg(nullptr, m_shutdownType, m_windowed, screen);
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
            auto *me = static_cast<QMouseEvent *>(event);
            if (std::ranges::any_of(m_dialogs, [me](KSMShutdownDlg *dialog) {
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
