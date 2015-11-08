/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *  Copyright 2015 David Edmundson <davidedmundson@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <QSessionManager>
#include <QDebug>
#include <QMessageBox>

#include <KAboutData>

#include <kdbusservice.h>
#include <klocalizedstring.h>

#include "shellcorona.h"
#include "standaloneappcorona.h"
#include "shellmanager.h"


void plasmaMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    //if we get an openGL error, display it properly in a user visible way.
    //Qt qFatals when failing to make an openGL context; we can't catch that earlier, so we do it in the error handler and display an error message
    //otherwise we continually get bug reports
    if (Q_UNLIKELY(type == QtFatalMsg)) {
            if (message == QLatin1String("Could not initialize GLX") ||
                    message == QLatin1String("QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled")) {
                QCoreApplication::setAttribute(Qt::AA_ForceRasterWidgets);
                QMessageBox::critical(nullptr, i18n("Plasma Failed To Start"), i18n("Plasma is unable to start as it could not correctly use OpenGL 2.\nPlease check that your graphic drivers are set up correctly."));
        }
    }

    //we have to reimplement the behaviour of the default message handler, as now we are always overriding it, and there's no way to access it
    const QString logMessage = qFormatLogMessage(type, context, message);
    fprintf(stderr, "%s\n", logMessage.toLocal8Bit().constData());
    fflush(stderr);
}

int main(int argc, char *argv[])
{
//    Devive pixel ratio has some problems in plasmashell currently.
//     - dialog continually expands (347951)
//     - Text element text is screwed (QTBUG-42606)
//     - Panel struts (350614)
//  This variable should possibly be removed when all are fixed

    qunsetenv("QT_DEVICE_PIXEL_RATIO");


    QQuickWindow::setDefaultAlphaBuffer(true);

    KLocalizedString::setApplicationDomain("plasmashell");

    QApplication app(argc, argv);

    KAboutData aboutData(QStringLiteral("plasmashell"),
                         i18n("Plasma"),
                         QStringLiteral(PROJECT_VERSION),
                         i18n("Plasma Shell"),
                         KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);

    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("plasma")));

    QCommandLineParser cliOptions;
    cliOptions.addHelpOption();
    cliOptions.addVersionOption();

    QCommandLineOption dbgOption(QStringList() << QStringLiteral("d") <<
                                 QStringLiteral("qmljsdebugger"),
                                 i18n("Enable QML Javascript debugger"));

    QCommandLineOption winOption(QStringList() << QStringLiteral("w") <<
                                 QStringLiteral("windowed"),
                                 i18n("Force a windowed view for testing purposes"));

    QCommandLineOption respawnOption(QStringList() << QStringLiteral("n") <<
                                     QStringLiteral("no-respawn"),
                                     i18n("Do not restart plasma-shell automatically after a crash"));

    QCommandLineOption crashOption(QStringList() << QStringLiteral("c") << QStringLiteral("crashes"),
                                   i18n("Recent number of crashes"),
                                   QStringLiteral("n"));

    QCommandLineOption shutupOption(QStringList() << QStringLiteral("s") << QStringLiteral("shut-up"),
                                 i18n("Deprecated, does nothing"));

    QCommandLineOption shellPluginOption(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                         i18n("Force loading the given shell plugin"),
                                         QStringLiteral("plugin"));

    QCommandLineOption standaloneOption(QStringList() << QStringLiteral("a") << QStringLiteral("standalone"),
                                        i18n("Load plasmashell as a standalone application, needs the shell-plugin option to be specified"));

    cliOptions.addOption(dbgOption);
    cliOptions.addOption(winOption);
    cliOptions.addOption(respawnOption);
    cliOptions.addOption(crashOption);
    cliOptions.addOption(shutupOption);
    cliOptions.addOption(shellPluginOption);
    cliOptions.addOption(standaloneOption);

    cliOptions.process(app);

    KAboutData::applicationData().setupCommandLine(&cliOptions);

    qInstallMessageHandler(plasmaMessageHandler);

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    ShellManager::s_crashes = cliOptions.value(crashOption).toInt();
    ShellManager::s_forceWindowed = cliOptions.isSet(winOption);
    ShellManager::s_noRespawn = cliOptions.isSet(respawnOption);
    ShellManager::s_fixedShell = cliOptions.value(shellPluginOption);

    if (cliOptions.isSet(dbgOption)) {
        ShellManager::s_restartOptions += " -" + dbgOption.names().first();
    }

    if (cliOptions.isSet(winOption)) {
        ShellManager::s_restartOptions += " -" + winOption.names().first();
    }

    if (cliOptions.isSet(respawnOption)) {
        ShellManager::s_restartOptions += " -" + respawnOption.names().first();
    }

    if (cliOptions.isSet(shutupOption)) {
        ShellManager::s_restartOptions += " -" + shutupOption.names().first();
    }

    if (cliOptions.isSet(shellPluginOption)) {
        ShellManager::s_restartOptions += " -" + shellPluginOption.names().first() + " " + ShellManager::s_fixedShell;
    }

    if (cliOptions.isSet(standaloneOption)) {
        if (cliOptions.isSet(shellPluginOption)) {
            ShellManager::s_standaloneOption = true;
            app.setApplicationName("plasmashell_"+cliOptions.value(shellPluginOption));
            app.setQuitOnLastWindowClosed(true);

            KDBusService service(KDBusService::Unique);
            //This will not leak, because corona deletes itself on window close
            new StandaloneAppCorona(cliOptions.value(shellPluginOption));
            return app.exec();
        } else {
            cliOptions.showHelp(1);
        }
    }

    KDBusService service(KDBusService::Unique);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, ShellManager::instance(), &QObject::deleteLater);

    return app.exec();
}
