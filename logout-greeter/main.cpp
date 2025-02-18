/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2016 Martin Graesslin <mgraesslin@kde.org>

    SPDX-License-Identifier: MIT
*/
#include <KConfigGroup>
#include <KPackage/PackageLoader>
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

QString shellPackage()
{
    KSharedConfig::Ptr startupConf = KSharedConfig::openConfig(QStringLiteral("plasmashellrc"));
    KConfigGroup startupConfGroup(startupConf, QStringLiteral("Shell"));
    const QString defaultValue = qEnvironmentVariable("PLASMA_DEFAULT_SHELL", QStringLiteral("org.kde.plasma.desktop"));
    QString value = startupConfGroup.readEntry("ShellPackage", defaultValue);

    // In the global theme an empty value was written, make sure we still return a shell package
    return value.isEmpty() ? defaultValue : value;
}

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
    {
        QCommandLineParser parser;
        QCommandLineOption testingOption(u"windowed"_s, u"have the dialog show, windowed, regardless of the session state"_s);
        parser.addOption(testingOption);
        parser.addHelpOption();
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

    const QString packageName = shellPackage();
    const auto pkg = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/Shell"), packageName);

    if (!pkg.isValid()) {
        qCWarning(LOGOUT_GREETER) << "Failed to load desktop package" << packageName;
    }

    Greeter greeter(pkg);
    if (windowed) {
        greeter.enableWindowed();
    }

    return app.exec();
}
