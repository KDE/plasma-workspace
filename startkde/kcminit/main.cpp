/*
    SPDX-FileCopyrightText: 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-workspace.h>

#include "main.h"

#include <unistd.h>

#include <KFileUtils>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QLibrary>
#include <QPluginLoader>
#include <QTimer>

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <kworkspace.h>

static int ready[2];
static bool startup = false;

static void sendReady()
{
    if (ready[1] == -1)
        return;
    char c = 0;
    write(ready[1], &c, 1);
    close(ready[1]);
    ready[1] = -1;
}

static void waitForReady()
{
    char c = 1;
    close(ready[1]);
    read(ready[0], &c, 1);
    close(ready[0]);
}

bool KCMInit::runModule(const KPluginMetaData &data)
{
    QString path = QPluginLoader(data.fileName()).fileName();

    // get the kcminit_ function
    QFunctionPointer init = QLibrary::resolve(path, "kcminit");
    if (!init) {
        qWarning() << "Module" << data.fileName() << "does not actually have a kcminit function";
        return false;
    }

    // initialize the module
    qDebug() << "Initializing " << data.fileName();
    init();
    return true;
}

void KCMInit::runModules(int phase)
{
    for (const KPluginMetaData &data : std::as_const(m_list)) {
        // see ksmserver's README for the description of the phases
        int libphase = data.value(QStringLiteral("X-KDE-Init-Phase"), 1);

        if (libphase > 1) {
            libphase = 1;
        }

        if (phase != -1 && libphase != phase)
            continue;

        // try to load the library
        if (!m_alreadyInitialized.contains(data.pluginId())) {
            runModule(data);
            m_alreadyInitialized.append(data.pluginId());
        }
    }
}

KCMInit::KCMInit(const QCommandLineParser &args)
{
    if (args.isSet(QStringLiteral("list"))) {
        m_list = KPluginMetaData::findPlugins(QStringLiteral("plasma/kcminit"));
        for (const KPluginMetaData &data : std::as_const(m_list)) {
            printf("%s\n", QFile::encodeName(data.fileName()).data());
        }
        return;
    }

    const auto positionalArguments = args.positionalArguments();
    if (!positionalArguments.isEmpty()) {
        for (const auto &arg : positionalArguments) {
            KPluginMetaData data(arg);
            if (!data.isValid()) {
                data = KPluginMetaData::findPluginById(QStringLiteral("plasma/kcminit"), arg);
            }

            if (data.isValid()) {
                m_list << data.fileName();
            } else {
                qWarning() << "Could not find" << arg;
            }
        }
    } else {
        m_list = KPluginMetaData::findPlugins(QStringLiteral("plasma/kcminit"));
    }

    if (startup) {
        runModules(0);
        // Tell KSplash that KCMInit has started
        QDBusMessage ksplashProgressMessage = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KSplash"),
                                                                             QStringLiteral("/KSplash"),
                                                                             QStringLiteral("org.kde.KSplash"),
                                                                             QStringLiteral("setStage"));
        ksplashProgressMessage.setArguments(QList<QVariant>() << QStringLiteral("kcminit"));
        QDBusConnection::sessionBus().asyncCall(ksplashProgressMessage);

        sendReady();
        QTimer::singleShot(300 * 1000, qApp, &QCoreApplication::quit); // just in case

        QDBusConnection::sessionBus().registerObject(QStringLiteral("/kcminit"), this, QDBusConnection::ExportScriptableContents);
        QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.kcminit"));

        qApp->exec(); // wait for runPhase1()
    } else
        runModules(-1); // all phases
}

KCMInit::~KCMInit()
{
    sendReady();
}

void KCMInit::runPhase1()
{
    runModules(1);
    qApp->exit(0);
}

int main(int argc, char *argv[])
{
    // plasma-session startup waits for kcminit to finish running phase 0 kcms
    // (theoretically that is only important kcms that need to be started very
    // early in the login process), the rest is delayed, so fork and make parent
    // return after the initial phase
    pipe(ready);
    if (fork() != 0) {
        waitForReady();
        return 0;
    }
    close(ready[0]);

    const QString executableName = QString::fromUtf8(argv[0]);
    startup = executableName.endsWith(QLatin1String("kcminit_startup")); // started from startkde?

    KWorkSpace::detectPlatform(argc, argv);
    QGuiApplication::setDesktopSettingsAware(false);
    QGuiApplication app(argc, argv); // gui is needed for several modules
    KLocalizedString::setApplicationDomain("kcminit");
    KAboutData about(QStringLiteral("kcminit"),
                     i18n("KCMInit"),
                     QString(),
                     i18n("KCMInit - runs startup initialization for Control Modules."),
                     KAboutLicense::GPL);
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("list"), i18n("List modules that are run at startup")));
    parser.addPositionalArgument(QStringLiteral("module"), i18n("Configuration module to run"));

    parser.process(app);
    about.processCommandLine(&parser);

    KCMInit kcminit(parser);
    return 0;
}
