/*
    SPDX-FileCopyrightText: 2012 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "debug.h"
#include "shellcorona.h"
#include "softwarerendernotifier.h"
#ifdef WITH_KUSERFEEDBACKCORE
#include "userfeedback.h"
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDebug>
#include <QDir>
#include <QLoggingCategory>
#include <QMessageBox>
#include <QProcess>
#include <QQmlDebuggingEnabler>
#include <QQuickWindow>
#include <QSessionManager>
#include <QSurfaceFormat>

#include <KAboutData>
#include <KCrash>
#include <KDBusService>
#include <KLocalizedString>
#include <KSignalHandler>

#include <csignal>

int main(int argc, char *argv[])
{
#if QT_CONFIG(qml_debug)
    if (qEnvironmentVariableIsSet("PLASMA_ENABLE_QML_DEBUG")) {
        QQmlDebuggingEnabler::enableDebugging(true);
    }
#endif

    auto format = QSurfaceFormat::defaultFormat();
    format.setOption(QSurfaceFormat::ResetNotification);
    QSurfaceFormat::setDefaultFormat(format);

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QQuickWindow::setDefaultAlphaBuffer(true);

    // this works around a bug of Qt and the plasmashell protocol
    // consider disabling when layer-shell lands
    qputenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS", {});
    // this variable controls whether to reconnect or exit if the compositor dies, given plasmashell does a lot of
    // bespoke wayland code disable for now. consider disabling when layer-shell lands
    qunsetenv("QT_WAYLAND_RECONNECT");
    QApplication app(argc, argv);

    qunsetenv("QT_WAYLAND_DISABLE_FIXED_POSITIONS");
    qputenv("QT_WAYLAND_RECONNECT", "1");

    KCrash::initialize();

    // Quit on SIGTERM to properly save state. See systemd.kill(5).
    // https://bugs.kde.org/show_bug.cgi?id=470604
    KSignalHandler::self()->watchSignal(SIGTERM);
    QObject::connect(KSignalHandler::self(), &KSignalHandler::signalReceived, &app, [&app](int signal) {
        if (signal == SIGTERM) {
            app.quit();
        }
    });

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("plasmashell"));

    // The executable's path is added to the library/plugin paths.
    // This does not make much sense for plasmashell.
    app.removeLibraryPath(QCoreApplication::applicationDirPath());

    KAboutData aboutData(QStringLiteral("plasmashell"), QString(), QStringLiteral(PROJECT_VERSION), i18n("Plasma Shell"), KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);

    app.setQuitOnLastWindowClosed(false);

    bool replace = false;

    ShellCorona *corona = nullptr;
    {
        QCommandLineParser cliOptions;

        QCommandLineOption dbgOption(QStringList() << QStringLiteral("d") << QStringLiteral("qmljsdebugger"), i18n("Enable QML Javascript debugger"));

        QCommandLineOption noRespawnOption(QStringList() << QStringLiteral("n") << QStringLiteral("no-respawn"),
                                           i18n("Do not restart plasma-shell automatically after a crash"));

        QCommandLineOption shellPluginOption(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                             i18n("Force loading the given shell plugin"),
                                             QStringLiteral("plugin"),
                                             ShellCorona::defaultShell());

        QCommandLineOption replaceOption({QStringLiteral("replace")}, i18n("Replace an existing instance"));

#ifdef WITH_KUSERFEEDBACKCORE
        QCommandLineOption feedbackOption(QStringList() << QStringLiteral("feedback"), i18n("Lists the available options for user feedback"));
#endif
        cliOptions.addOption(dbgOption);
        cliOptions.addOption(noRespawnOption);
        cliOptions.addOption(shellPluginOption);
        cliOptions.addOption(replaceOption);
#ifdef WITH_KUSERFEEDBACKCORE
        cliOptions.addOption(feedbackOption);
#endif

        aboutData.setupCommandLine(&cliOptions);
        cliOptions.process(app);
        aboutData.processCommandLine(&cliOptions);

        // don't let the first KJob terminate us
        QCoreApplication::setQuitLockEnabled(false);

        auto disableSessionManagement = [](QSessionManager &sm) {
            sm.setRestartHint(QSessionManager::RestartNever);
        };
        QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
        QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

        corona = new ShellCorona(&app);
        corona->setShell(cliOptions.value(shellPluginOption));
        if (!corona->kPackage().isValid()) {
            qCritical() << "starting invalid corona" << corona->shell();
            return 1;
        }

#ifdef WITH_KUSERFEEDBACKCORE
        auto userFeedback = new UserFeedback(corona, &app);
        if (cliOptions.isSet(feedbackOption)) {
            QTextStream(stdout) << userFeedback->describeDataSources();
            return 0;
        }
#endif

        QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, corona, &QObject::deleteLater);

        if (!cliOptions.isSet(noRespawnOption)) {
            KCrash::setFlags(KCrash::AutoRestart);
        }

        // Tells libnotificationmanager that we're the only true application that may own notification and job progress services
        qApp->setProperty("_plasma_dbus_master", true);

        QObject::connect(corona, &ShellCorona::glInitializationFailed, &app, [&app]() {
            // scene graphs errors come from a thread
            // even though we process them in the main thread, app.exit could still process these events
            static bool s_multipleInvokations = false;
            if (s_multipleInvokations) {
                return;
            }
            s_multipleInvokations = true;

            qCritical("Open GL context could not be created");
            auto configGroup = KSharedConfig::openConfig()->group(QStringLiteral("QtQuickRendererSettings"));
            if (configGroup.readEntry("SceneGraphBackend") != QLatin1String("software")) {
                configGroup.writeEntry("SceneGraphBackend", "software", KConfigBase::Global | KConfigBase::Persistent);
                configGroup.sync();
                QProcess::startDetached(QStringLiteral("plasmashell"), app.arguments());
            } else {
                QCoreApplication::setAttribute(Qt::AA_ForceRasterWidgets);
                QMessageBox::critical(nullptr,
                                      i18n("Plasma Failed To Start"),
                                      i18n("Plasma is unable to start as it could not correctly use OpenGL 2 or software fallback\nPlease check that your "
                                           "graphic drivers are set up correctly."));
            }
            app.exit(-1);
        });
        replace = cliOptions.isSet(replaceOption);
    }

    KDBusService service(KDBusService::Unique | KDBusService::StartupOption(replace ? KDBusService::Replace : 0));

    corona->init();
    SoftwareRendererNotifier::notifyIfRelevant();

    return app.exec();
}
