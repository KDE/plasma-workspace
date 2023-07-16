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

#include <kworkspace.h>

#include "view.h"

int main(int argc, char **argv)
{
    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QCommandLineParser parser;

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    bool usingPlasmaShellIntegration = false;
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY") || qEnvironmentVariableIsSet("WAYLAND_SOCKET")) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "plasma-shell");
        usingPlasmaShellIntegration = true;
    }
#endif

    // this is needed to fake window position so Plasma Dialog sets correct borders
    qputenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS", {});
    // this variable controls whether to reconnect or exit if the compositor dies, given plasmashell does a lot of
    // bespoke wayland code disable for now. Consider re-enabling when layer-shell support lands
    qunsetenv("QT_WAYLAND_RECONNECT");
    const bool qpaVariable = qEnvironmentVariableIsSet("QT_QPA_PLATFORM");
    KWorkSpace::detectPlatform(argc, argv);
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);
    if (!qpaVariable) {
        // don't leak the env variable to processes we start
        qunsetenv("QT_QPA_PLATFORM");
    }
    qunsetenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS");
    qputenv("QT_WAYLAND_RECONNECT", "1");

#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
    if (usingPlasmaShellIntegration) {
        // Force qtwayland to load our plasma-shell integration plugin. qtwayland loads
        // the shell integration plugin on demand, we need to create a window.
        QWindow w;
        w.create();
        qunsetenv("QT_WAYLAND_SHELL_INTEGRATION");
    }
#endif

    KLocalizedString::setApplicationDomain("krunner");

    //     TODO: Make it a QGuiApplication once we don't depend on KDELibs4Support
    //     QGuiApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("krunner"), i18n("KRunner"), QStringLiteral(PROJECT_VERSION), i18n("Run Command interface"), KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);
    app.setQuitOnLastWindowClosed(false);
    app.setQuitLockEnabled(false);

    QCommandLineOption clipboardOption({QStringLiteral("c"), QStringLiteral("clipboard")}, i18n("Use the clipboard contents as query for KRunner"));
    QCommandLineOption daemonOption({QStringLiteral("d"), QStringLiteral("daemon")}, i18n("Start KRunner in the background, don't show it."));
    QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));
    QCommandLineOption runnerId({QStringLiteral("runner")}, i18n("Only query this specific runner"), QStringLiteral("runner"));

    parser.addOption(clipboardOption);
    parser.addOption(daemonOption);
    parser.addOption(replaceOption);
    parser.addOption(runnerId);
    parser.addPositionalArgument(QStringLiteral("query"), i18n("The query to run, only used if -c is not provided"));

    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (!KAuthorized::authorize(QStringLiteral("run_command"))) {
        return -1;
    }

    KDBusService service(KDBusService::Unique | KDBusService::StartupOption(parser.isSet(replaceOption) ? KDBusService::Replace : 0));

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    View view;

    auto updateVisibility = [&]() {
        const QString query = parser.positionalArguments().value(0);
        const bool hasSingleRunnerModeId = parser.isSet(runnerId);

        if (parser.isSet(daemonOption)) {
            view.setVisible(false);
        } else if (parser.isSet(clipboardOption)) {
            view.displayWithClipboardContents();
        } else if (!query.isEmpty()) {
            if (hasSingleRunnerModeId) {
                view.querySingleRunner(parser.value(runnerId), query);
            } else {
                view.query(query);
            }
        } else if (hasSingleRunnerModeId) {
            view.displaySingleRunner(parser.value(runnerId));
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
