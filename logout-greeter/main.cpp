/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>

    SPDX-License-Identifier: MIT
*/
#include <KConfigGroup>
#include <KSharedConfig>
#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QLibraryInfo>
#include <QQuickWindow>
#include <QSurfaceFormat>

#include "debug.h"
#include "greeter.h"

#include <sessionmanagement.h>

using namespace Qt::StringLiterals;

int main(int argc, char *argv[])
{
    qunsetenv("SESSION_MANAGER");

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QQuickWindow::setDefaultAlphaBuffer(true);
    QGuiApplication app(argc, argv);

    bool windowed = false;
    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), u"KDE"_s);
    QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    {
        QCommandLineParser parser;
        QCommandLineOption testingOption("windowed", "have the dialog show, windowed, regardless of the session state");
        QCommandLineOption lnfOption("lookandfeel", "The look and feel package name to use", "name", packageName);
        parser.addOption(testingOption);
        parser.addOption(lnfOption);
        parser.addHelpOption();
        parser.process(app);
        windowed = parser.isSet(testingOption);
        if (parser.isSet(lnfOption)) {
            packageName = parser.value(lnfOption);
        }
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

    const auto pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"), packageName);

    if (!pkg.isValid()) {
        qCWarning(LOGOUT_GREETER) << "Failed to load lookandfeel package" << packageName;
    }

    Greeter greeter(pkg);
    if (windowed) {
        greeter.enableWindowed();
    }

    return app.exec();
}
