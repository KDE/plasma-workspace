/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KLocalizedString>
#include <QApplication>

#include <QAction>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QQuickWindow>
#include <QSessionManager>
#include <QSurfaceFormat>
#include <QUrl>

#include <KAboutData>
#include <KAuthorized>
#include <KDBusService>

#include <kdeclarative/qmlobject.h>

#include <kworkspace.h>

#include "view.h"

int main(int argc, char **argv)
{
    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QCommandLineParser parser;
    if (!qEnvironmentVariableIsSet("PLASMA_USE_QT_SCALING")) {
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    }

    qputenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS", {});
    const bool qpaVariable = qEnvironmentVariableIsSet("QT_QPA_PLATFORM");
    KWorkSpace::detectPlatform(argc, argv);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);
    if (!qpaVariable) {
        // don't leak the env variable to processes we start
        qunsetenv("QT_QPA_PLATFORM");
    }
    qunsetenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS");
    KLocalizedString::setApplicationDomain("krunner");

    //     TODO: Make it a QGuiApplication once we don't depend on KDELibs4Support
    //     QGuiApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("krunner"), i18n("KRunner"), QStringLiteral(PROJECT_VERSION), i18n("Run Command interface"), KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);
    app.setQuitOnLastWindowClosed(false);

    QCommandLineOption clipboardOption({QStringLiteral("c"), QStringLiteral("clipboard")}, i18n("Use the clipboard contents as query for KRunner"));
    QCommandLineOption daemonOption({QStringLiteral("d"), QStringLiteral("daemon")}, i18n("Start KRunner in the background, don't show it."));
    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));

    parser.addOption(clipboardOption);
    parser.addOption(daemonOption);
    parser.addOption(replaceOption);
    parser.addPositionalArgument(QStringLiteral("query"), i18n("The query to run, only used if -c is not provided"));

    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!KAuthorized::authorize(QStringLiteral("run_command"))) {
        return -1;
    }

    KDBusService service(KDBusService::Unique | KDBusService::StartupOption(parser.isSet(replaceOption) ? KDBusService::Replace : 0));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setFallbackSessionManagementEnabled(false);
#endif

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    View view;

    auto updateVisibility = [&]() {
        const QString query = parser.positionalArguments().value(0);

        if (parser.isSet(daemonOption)) {
            view.setVisible(false);
        } else if (parser.isSet(clipboardOption)) {
            view.displayWithClipboardContents();
        } else if (!query.isEmpty()) {
            view.query(query);
        } else {
            view.toggleDisplay();
        }
    };

    updateVisibility();

    QObject::connect(&service, &KDBusService::activateRequested, &view, [&](const QStringList &arguments, const QString &workingDirectory) {
        Q_UNUSED(workingDirectory)
        parser.parse(arguments);
        updateVisibility();
    });
    QObject::connect(&service, &KDBusService::activateActionRequested, &view, [&view](const QString &action) {
        if (action == QLatin1String("RunClipboard")) {
            view.displayWithClipboardContents();
        }
    });

    return app.exec();
}
