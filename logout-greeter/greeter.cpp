/*****************************************************************
ksmserver - the KDE session management server

Copyright 2016 Martin Graesslin <mgraesslin@kde.org>
Copyright 2018 David Edmundson <davidedmundson@kde.org>

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

#include "greeter.h"

#include <QDebug>
#include <QScreen>
#include <QApplication>

#include "shutdowndlg.h"

#include "logoutpromptadaptor.h"
#include "ksmserveriface.h"

#include <KQuickAddons/QtQuickSettings>
#include <KWindowSystem>

#include <KWayland/Client/connection_thread.h>
#include <KWayland/Client/registry.h>
#include <KWayland/Client/plasmashell.h>

Greeter::Greeter(bool shutdownAllowed)
    : QObject()
    , m_shutdownAllowed(shutdownAllowed)
    , m_waylandPlasmaShell(nullptr)
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
    if (!KWindowSystem::isPlatformWayland()) {
        return;
    }
    using namespace KWayland::Client;
    ConnectionThread *connection = ConnectionThread::fromApplication(this);
    if (!connection) {
        return;
    }
    Registry *registry = new Registry(this);
    registry->create(connection);
    connect(registry, &Registry::plasmaShellAnnounced, this,
        [this, registry] (quint32 name, quint32 version) {
            m_waylandPlasmaShell = registry->createPlasmaShell(name, version, this);
        }
    );
    registry->setup();
    connection->roundtrip();
}

void Greeter::init()
{
    setupWaylandIntegration();
    const auto screens = qApp->screens();
    for (QScreen *screen: screens) {
        adoptScreen(screen);
    }
    connect(qApp, &QGuiApplication::screenAdded, this, &Greeter::adoptScreen);
    m_running = true;
}

void Greeter::adoptScreen(QScreen* screen)
{
    // TODO: last argument is the theme, maybe add command line option for it?
    KSMShutdownDlg *w = new KSMShutdownDlg(nullptr, m_shutdownAllowed, m_shutdownType, m_waylandPlasmaShell);
    w->installEventFilter(this);
    m_dialogs << w;

    QObject::connect(screen, &QObject::destroyed, w, [w, this] {
        m_dialogs.removeOne(w);
        w->deleteLater();
    });
    connect(w, &KSMShutdownDlg::rejected, this, &Greeter::rejected);
    connect(w, &KSMShutdownDlg::accepted, this, [w]() {
        OrgKdeKSMServerInterfaceInterface ksmserver(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
        ksmserver.logout(KWorkSpace::ShutdownConfirmNo, w->shutdownType(), KWorkSpace::ShutdownModeDefault);
        QApplication::exit(1);
    });
    w->setScreen(screen);
    w->setGeometry(screen->geometry());
    w->init();
}

void Greeter::rejected()
{
    QApplication::exit(1);
}

bool Greeter::eventFilter(QObject *watched, QEvent *event)
{
    if (qobject_cast<KSMShutdownDlg*>(watched)) {
        if (event->type() == QEvent::MouseButtonPress) {
            // check that the position is on no window
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            for (auto it = m_dialogs.constBegin(); it != m_dialogs.constEnd(); ++it) {
                if ((*it)->geometry().contains(me->globalPos())) {
                    return false;
                }
            }
            // click outside, close
            rejected();
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


