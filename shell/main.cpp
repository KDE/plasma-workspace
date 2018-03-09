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
    //Plasma scales itself to font DPI
    //on X, where we don't have compositor scaling, this generally works fine.
    //also there are bugs on older Qt, especially when it comes to fractional scaling
    //there's advantages to disabling, and (other than small context menu icons) few advantages in enabling

    //On wayland, it's different. Everything is simpler as all co-ordinates are in the same co-ordinate system
    //we don't have fractional scaling on the client so don't hit most the remaining bugs and
    //even if we don't use Qt scaling the compositor will try to scale us anyway so we have no choice
    if (!qEnvironmentVariableIsSet("PLASMA_USE_QT_SCALING")) {
        qunsetenv("QT_DEVICE_PIXEL_RATIO");
        QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    } else {
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    }

    QQuickWindow::setDefaultAlphaBuffer(true);

    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("plasmashell");

    // The executable's path is added to the library/plugin paths.
    // This does not make much sense for plasmashell.
    app.removeLibraryPath(QCoreApplication::applicationDirPath());

    KQuickAddons::QtQuickSettings::init();

    KAboutData aboutData(QStringLiteral("plasmashell"),
                         i18n("Plasma"),
                         QStringLiteral(PROJECT_VERSION),
                         i18n("Plasma Shell"),
                         KAboutLicense::GPL);

    KAboutData::setApplicationData(aboutData);

    app.setQuitOnLastWindowClosed(false);

    {
    QCommandLineParser cliOptions;

    QCommandLineOption dbgOption(QStringList() << QStringLiteral("d") <<
                                 QStringLiteral("qmljsdebugger"),
                                 i18n("Enable QML Javascript debugger"));

    QCommandLineOption noRespawnOption(QStringList() << QStringLiteral("n") <<
                                     QStringLiteral("no-respawn"),
                                     i18n("Do not restart plasma-shell automatically after a crash"));

    QCommandLineOption shellPluginOption(QStringList() << QStringLiteral("p") << QStringLiteral("shell-plugin"),
                                         i18n("Force loading the given shell plugin"),
                                         QStringLiteral("plugin"));

    QCommandLineOption standaloneOption(QStringList() << QStringLiteral("a") << QStringLiteral("standalone"),
                                        i18n("Load plasmashell as a standalone application, needs the shell-plugin option to be specified"));


    QCommandLineOption testOption(QStringList() << QStringLiteral("test"),
                                        i18n("Enables test mode and specifies the layout javascript file to set up the testing environment"), i18n("file"), QStringLiteral("layout.js"));

    cliOptions.addOption(dbgOption);
    cliOptions.addOption(noRespawnOption);
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

    if (!cliOptions.isSet(noRespawnOption) && !cliOptions.isSet(testOption)) {
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

        qApp->setProperty("org.kde.KActivities.core.disableAutostart", true);

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
