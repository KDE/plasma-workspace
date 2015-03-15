/*
 *  Copyright 2012 Marco Martin <mart@kde.org>
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
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
#include <KAboutData>

#include <kdbusservice.h>
#include <klocalizedstring.h>

#include "shellcorona.h"
#include "standaloneappcorona.h"
#include "shellmanager.h"

void noMessageOutput(QtMsgType type, const char *msg)
{
     Q_UNUSED(type);
     Q_UNUSED(msg);
}

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    KLocalizedString::setApplicationDomain("plasmashell");

    QApplication app(argc, argv);

    KAboutData aboutData("plasmashell",
                         i18n("Plasma Shell"),
                         QStringLiteral(PROJECT_VERSION),
                         i18n("Plasma Shell"),
                         KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);

    app.setApplicationDisplayName(i18n("Plasma"));
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon::fromTheme("plasma"));

    QCommandLineParser cliOptions;
    cliOptions.addHelpOption();

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
                                    i18n("Shuts up the output"));

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

    if (cliOptions.isSet(shutupOption)) {
        qInstallMsgHandler(noMessageOutput);
    }

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

    QObject::connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), ShellManager::instance(), SLOT(deleteLater()));

    return app.exec();
}
