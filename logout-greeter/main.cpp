/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>

    SPDX-License-Identifier: MIT
*/
#include <QApplication>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QQuickWindow>
#include <QSurfaceFormat>

#include "ksmserveriface.h"

#include "greeter.h"

#include <sessionmanagement.h>

int main(int argc, char *argv[])
{
    qunsetenv("SESSION_MANAGER");

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    KWorkSpace::detectPlatform(argc, argv);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QGuiApplication app(argc, argv);

    OrgKdeKSMServerInterfaceInterface ksmserver(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
    QDBusPendingReply<bool> isShuttingDownPending = ksmserver.isShuttingDown();

    isShuttingDownPending.waitForFinished();

    // if ksmserver is shutting us down already, we don't want another prompt
    if (isShuttingDownPending.value()) {
        return 0;
    }

    bool windowed = false;
    {
        QCommandLineParser parser;
        QCommandLineOption testingOption("windowed", "have the dialog show, windowed, regardless of the session state");
        parser.addOption(testingOption);
        parser.process(app);
        windowed = parser.isSet(testingOption);
    }

    // because we export stuff as horrific contextProperties we need to know "maysd" may shutdown, at the time of initial creation and can't update
    // later.
    // Force the backend to load everything now, then the shared backend will be cached when a new object is created later

    // TODO Plasma 6, just have the greeter QML import and use the SessionManagement object directly
    // We don't need any special slot handling in ShutdownDlg
    SessionManagement m_session;
    if (m_session.state() == SessionManagement::State::Loading) {
        QEventLoop e;
        QObject::connect(&m_session, &SessionManagement::stateChanged, &e, &QEventLoop::quit);
        e.exec();
    }

    Greeter greeter;
    if (windowed) {
        greeter.enableWindowed();
    }

    return app.exec();
}
