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
#include <KRunner/RunnerManager>

#include <PlasmaQuick/SharedQmlEngine>

#include <iostream>
#include <qqmlengine.h>

#include "view.h"

using namespace Qt::StringLiterals;

int main(int argc, char **argv)
{
    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QCommandLineParser parser;

    // this is needed to fake window position so Plasma Dialog sets correct borders
    qputenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS", {});
    // this variable controls whether to reconnect or exit if the compositor dies, given plasmashell does a lot of
    // bespoke wayland code disable for now. Consider re-enabling when layer-shell support lands
    qunsetenv("QT_WAYLAND_RECONNECT");
    QQuickWindow::setDefaultAlphaBuffer(true);
    QApplication app(argc, argv);
    qunsetenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS");
    qputenv("QT_WAYLAND_RECONNECT", "1");

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("krunner"));

    //     TODO: Make it a QGuiApplication once we don't depend on KDELibs4Support
    //     QGuiApplication app(argc, argv);

    KAboutData aboutData(u"krunner"_s, i18n("KRunner"), QStringLiteral(PROJECT_VERSION), i18n("Run Command interface"), KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);
    app.setQuitOnLastWindowClosed(false);
    app.setQuitLockEnabled(false);

    QCommandLineOption clipboardOption({u"c"_s, u"clipboard"_s}, i18n("Use the clipboard contents as query for KRunner"));
    QCommandLineOption daemonOption({u"d"_s, u"daemon"_s}, i18n("Start KRunner in the background, don't show it."));
    QCommandLineOption replaceOption({u"replace"_s}, i18n("Replace an existing instance"));
    QCommandLineOption runnerId({u"runner"_s}, i18n("Show only results from the given plugin"), u"runner"_s);
    QCommandLineOption listOption({u"list"_s}, i18n("List available plugins"));

    parser.addOption(clipboardOption);
    parser.addOption(daemonOption);
    parser.addOption(replaceOption);
    parser.addOption(runnerId);
    parser.addOption(listOption);
    parser.addPositionalArgument(u"query"_s, i18n("The query to run, only used if -c is not provided"));

    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

    if (parser.isSet(listOption)) {
        const auto runners = KRunner::RunnerManager::runnerMetaDataList();
        const QString headline = i18nc("Header for command line output", "Available KRunner plugins, pluginId");
        QString separator;
        separator.fill(u'-', headline.length());
        std::cout << qPrintable(headline) << std::endl << qPrintable(separator) << std::endl;
        for (const auto &data : runners) {
            std::cout << qPrintable(data.name()) << ": " << qPrintable(data.pluginId()) << std::endl;
        }
        return 0;
    }

    if (!KAuthorized::authorize(u"run_command"_s)) {
        return -1;
    }

    KDBusService service(KDBusService::Unique | KDBusService::StartupOption(parser.isSet(replaceOption) ? KDBusService::Replace : 0));

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    PlasmaQuick::SharedQmlEngine sharedEngine;
    // It is important this to be done before the view is created, as it creates internally a framesvgitem for the background
    // that needs to use the current plasma theme
    sharedEngine.engine()->setProperty("_kirigamiTheme", u"KirigamiPlasmaStyle"_s);
    sharedEngine.setInitializationDelayed(true);
    View view(&sharedEngine);

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

    QObject::connect(&service, &KDBusService::activateRequested, &view, [&](const QStringList &arguments) {
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
