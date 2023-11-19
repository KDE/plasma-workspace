/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2018 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "greeter.h"

#include <QApplication>
#include <QDebug>
#include <QQmlEngine>
#include <QScreen>

#include "shutdowndlg.h"

#include "logoutpromptadaptor.h"

#include <KWindowSystem>

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
    promptLogout();
}

void Greeter::adoptScreen(QScreen *screen)
{
    if (screen->geometry().isNull()) {
        return;
    }
    // TODO: last argument is the theme, maybe add command line option for it?
    KSMShutdownDlg *w = new KSMShutdownDlg(nullptr, m_defaultAction, screen);
    w->setWindowed(m_windowed);
    w->installEventFilter(this);
    m_dialogs << w;

    QObject::connect(screen, &QObject::destroyed, w, [w, this] {
        m_dialogs.removeOne(w);
        w->deleteLater();
    });

    connect(w->engine().get(), &QQmlEngine::quit, qApp, &QCoreApplication::quit, Qt::QueuedConnection);

    w->setGeometry(screen->geometry());
    w->init(m_package);
}

void Greeter::promptLogout()
{
    if (m_running) {
        return;
    }
    m_defaultAction = QStringLiteral("logout");
    init();
}

void Greeter::promptShutDown()
{
    if (m_running) {
        return;
    }
    m_defaultAction = QStringLiteral("shutdown");
    init();
}

void Greeter::promptReboot()
{
    if (m_running) {
        return;
    }
    m_defaultAction = QStringLiteral("reboot");
    init();
}
