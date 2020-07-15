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
#include <QDebug>
#include <QProcess>
#include <QMessageBox>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QtQml/QQmlDebuggingEnabler>
#include <QLoggingCategory>

#include <KAboutData>
#include <KQuickAddons/QtQuickSettings>
#ifdef WITH_KUSERFEEDBACKCORE
#include <KUserFeedback/Provider>
#endif

#include <kdbusservice.h>
#include <klocalizedstring.h>
#include <kcrash.h>
#include <kworkspace.h>

#include "shellcorona.h"
#include "standaloneappcorona.h"
#include "coronatesthelper.h"
#include "softwarerendernotifier.h"

#include <QDir>
#include <QDBusConnectionInterface>

static QLoggingCategory::CategoryFilter oldCategoryFilter;

// Qt 5.15 introduces a new syntax for connections
// framework code can't port away due to needing Qt5.12
// this filters out the warnings
// Remove this once we depend on Qt5.15 in frameworks
void filterConnectionSyntaxWarning(QLoggingCategory *category)
{
    if (qstrcmp(category->categoryName(), "qt.qml.connections") == 0) {
        category->setEnabled(QtWarningMsg, false);
    } else if (oldCategoryFilter) {
        oldCategoryFilter(category);
    }
}

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsSet("PLASMA_ENABLE_QML_DEBUG")) {
        QQmlDebuggingEnabler debugger;
    }
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
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);

    QQuickWindow::setDefaultAlphaBuffer(true);

    oldCategoryFilter = QLoggingCategory::installFilter(filterConnectionSyntaxWarning);

    const bool qpaVariable = qEnvironmentVariableIsSet("QT_QPA_PLATFORM");
    KWorkSpace::detectPlatform(argc, argv);
    QApplication app(argc, argv);
    if (!qpaVariable) {
        // don't leak the env variable to processes we start
        qunsetenv("QT_QPA_PLATFORM");
    }
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

    KSharedConfig::Ptr startupConf = KSharedConfig::openConfig(QStringLiteral("plasmashellrc"));
    KConfigGroup startupConfGroup(startupConf, "Shell");
    const QString defaultShell = startupConfGroup.readEntry("ShellPackage", qEnvironmentVariable("PLASMA_DEFAULT_SHELL", "org.kde.plasma.desktop"));

    bool replace = false;
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
                                         QStringLiteral("plugin"), defaultShell);

    QCommandLineOption standaloneOption(QStringList() << QStringLiteral("a") << QStringLiteral("standalone"),
                                        i18n("Load plasmashell as a standalone application, needs the shell-plugin option to be specified"));

    QCommandLineOption replaceOption({QStringLiteral("replace")},
                                 i18n("Replace an existing instance"));

    QCommandLineOption testOption(QStringList() << QStringLiteral("test"),
                                        i18n("Enables test mode and specifies the layout javascript file to set up the testing environment"), i18n("file"), QStringLiteral("layout.js"));


#ifdef WITH_KUSERFEEDBACKCORE
    QCommandLineOption feedbackOption(QStringList() << QStringLiteral("feedback"),
                                        i18n("Lists the available options for user feedback"));
#endif
    cliOptions.addOption(dbgOption);
    cliOptions.addOption(noRespawnOption);
    cliOptions.addOption(shellPluginOption);
    cliOptions.addOption(standaloneOption);
    cliOptions.addOption(testOption);
    cliOptions.addOption(replaceOption);
#ifdef WITH_KUSERFEEDBACKCORE
    cliOptions.addOption(feedbackOption);
#endif

    aboutData.setupCommandLine(&cliOptions);
    cliOptions.process(app);
    aboutData.processCommandLine(&cliOptions);

    ShellCorona* corona = new ShellCorona(&app);
    corona->setShell(cliOptions.value(shellPluginOption));

#ifdef WITH_KUSERFEEDBACKCORE
    if (cliOptions.isSet(feedbackOption)) {
        QTextStream(stdout) << corona->feedbackProvider()->describeDataSources();
        return 0;
    }
#endif

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, corona, &QObject::deleteLater);

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
        corona->setTestModeLayout(layoutUrl.toLocalFile());

        qApp->setProperty("org.kde.KActivities.core.disableAutostart", true);

        new CoronaTestHelper(corona);
    }

    if (cliOptions.isSet(standaloneOption)) {
        if (cliOptions.isSet(shellPluginOption)) {
            app.setApplicationName(QStringLiteral("plasmashell_") + cliOptions.value(shellPluginOption));
            app.setQuitOnLastWindowClosed(true);

            KDBusService service(KDBusService::Unique);
            //This will not leak, because corona deletes itself on window close
            new StandaloneAppCorona(cliOptions.value(shellPluginOption));
            return app.exec();
        } else {
            cliOptions.showHelp(1);
        }
    } else {
        // Tells libnotificationmanager that we're the only true application that may own notification and job progress services
        qApp->setProperty("_plasma_dbus_master", true);
    }

    QObject::connect(corona, &ShellCorona::glInitializationFailed, &app, [&app]() {
        //scene graphs errors come from a thread
        //even though we process them in the main thread, app.exit could still process these events
        static bool s_multipleInvokations = false;
        if (s_multipleInvokations) {
            return;
        }
        s_multipleInvokations = true;

        qCritical("Open GL context could not be created");
        auto configGroup = KSharedConfig::openConfig()->group("QtQuickRendererSettings");
        if (configGroup.readEntry("SceneGraphBackend") != QLatin1String("software")) {
            configGroup.writeEntry("SceneGraphBackend", "software", KConfigBase::Global | KConfigBase::Persistent);
            configGroup.sync();
            QProcess::startDetached(QStringLiteral("plasmashell"), app.arguments());
        } else {
            QCoreApplication::setAttribute(Qt::AA_ForceRasterWidgets);
            QMessageBox::critical(nullptr, i18n("Plasma Failed To Start"),
                          i18n("Plasma is unable to start as it could not correctly use OpenGL 2 or software fallback\nPlease check that your graphic drivers are set up correctly."));
        }
        app.exit(-1);
    });
    replace = cliOptions.isSet(replaceOption);
    }

    KDBusService service(KDBusService::Unique | KDBusService::StartupOption(replace ? KDBusService::Replace : 0));

    SoftwareRendererNotifier::notifyIfRelevant();

    return app.exec();
}
