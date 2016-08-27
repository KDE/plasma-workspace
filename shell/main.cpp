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
#include <KQuickAddons/QtQuickSettings>

#include <kdbusservice.h>
#include <klocalizedstring.h>
#include <kcrash.h>

#include "shellcorona.h"
#include "standaloneappcorona.h"
#include "shellmanager.h"
#include "coronatesthelper.h"

#include <QDir>

int main(int argc, char *argv[])
{
//    Devive pixel ratio has some problems in plasmashell currently.
//     - dialog continually expands (347951)
//     - Text element text is screwed (QTBUG-42606)
//     - Panel struts (350614)
//  This variable should possibly be removed when all are fixed

    qunsetenv("QT_DEVICE_PIXEL_RATIO");
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    QQuickWindow::setDefaultAlphaBuffer(true);

    KLocalizedString::setApplicationDomain("plasmashell");

    QApplication app(argc, argv);

    KQuickAddons::QtQuickSettings::init();

    KAboutData aboutData(QStringLiteral("plasmashell"),
                         i18n("Plasma"),
                         QStringLiteral(PROJECT_VERSION),
                         i18n("Plasma Shell"),
                         KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);

    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("plasma")));

    {
    QCommandLineParser cliOptions;

    QCommandLineOption dbgOption(QStringList() << QStringLiteral("d") <<
                                 QStringLiteral("qmljsdebugger"),
                                 i18n("Enable QML Javascript debugger"));

    QCommandLineOption winOption(QStringList() << QStringLiteral("w") <<
                                 QStringLiteral("windowed"),
                                 i18n("Force a windowed view for testing purposes"));

    QCommandLineOption noRespawnOption(QStringList() << QStringLiteral("n") <<
                                     QStringLiteral("no-respawn"),
                                     i18n("Do not restart plasma-shell automatically after a crash"));

    QCommandLineOption shutupOption(QStringList() << QStringLiteral("s") << QStringLiteral("shut-up"),
                                 i18n("Deprecated, does nothing"));

    QCommandLineOption shellPluginOption(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                         i18n("Force loading the given shell plugin"),
                                         QStringLiteral("plugin"));

    QCommandLineOption standaloneOption(QStringList() << QStringLiteral("a") << QStringLiteral("standalone"),
                                        i18n("Load plasmashell as a standalone application, needs the shell-plugin option to be specified"));


    QCommandLineOption testOption(QStringList() << QStringLiteral("test"),
                                        i18n("Enables test mode and specifies the layout javascript file to set up the testing environment"), i18n("file"), QStringLiteral("layout.js"));

    cliOptions.addOption(dbgOption);
    cliOptions.addOption(winOption);
    cliOptions.addOption(noRespawnOption);
    cliOptions.addOption(shutupOption);
    cliOptions.addOption(shellPluginOption);
    cliOptions.addOption(standaloneOption);
    cliOptions.addOption(testOption);

    aboutData.setupCommandLine(&cliOptions);
    cliOptions.process(app);
    aboutData.processCommandLine(&cliOptions);

    QGuiApplication::setFallbackSessionManagementEnabled(false);

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);

    ShellManager::s_fixedShell = cliOptions.value(shellPluginOption);

    if (!cliOptions.isSet(noRespawnOption)) {
        KCrash::setFlags(KCrash::AutoRestart);
    }

    if (cliOptions.isSet(testOption)) {
        const QUrl layoutUrl = QUrl::fromUserInput(cliOptions.value(testOption), {}, QUrl::AssumeLocalFile);
        if (!layoutUrl.isLocalFile()) {
            qWarning() << "ensure the layout file is local" << layoutUrl;
            cliOptions.showHelp(1);
        }

        QStandardPaths::setTestModeEnabled(true);
        QDir(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)).removeRecursively();
        ShellManager::s_testModeLayout = layoutUrl.toLocalFile();

        QObject::connect(ShellManager::instance(), &ShellManager::shellChanged,
                         ShellManager::instance(),
                            [layoutUrl]() {
                                new CoronaTestHelper(ShellManager::instance()->corona());
                            }
                        );
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
    }

    KDBusService service(KDBusService::Unique);

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, ShellManager::instance(), &QObject::deleteLater);

    return app.exec();
}
