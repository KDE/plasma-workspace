/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <config-workspace.h>

#include "klauncher_iface.h"
#include "main.h"

#include <unistd.h>

#include <QDBusConnection>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QLibrary>
#include <QTimer>

#include <KAboutData>
#include <KConfig>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KServiceTypeTrader>
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

bool KCMInit::runModule(const QString &libName, KService::Ptr service)
{
    QString KCMINIT_PREFIX = QStringLiteral("kcminit_");
    const QVariant tmp = service->property(QStringLiteral("X-KDE-Init-Symbol"), QVariant::String);
    QString kcminit;
    if (tmp.isValid()) {
        kcminit = tmp.toString();
        if (!kcminit.startsWith(KCMINIT_PREFIX))
            kcminit = KCMINIT_PREFIX + kcminit;
    } else
        kcminit = KCMINIT_PREFIX + libName;

    QString path = KPluginLoader::findPlugin(libName);
    if (path.isEmpty()) {
        path = KPluginLoader::findPlugin(QStringLiteral("kcms/") + libName);
    }

    if (path.isEmpty()) {
        qWarning() << "Module" << libName << "was not found";
        return false;
    }

    // get the kcminit_ function
    QFunctionPointer init = QLibrary::resolve(path, kcminit.toUtf8().constData());
    if (!init) {
        qWarning() << "Module" << libName << "does not actually have a kcminit function";
        return false;
    }

    // initialize the module
    qDebug() << "Initializing " << libName << ": " << kcminit;
    init();
    return true;
}

void KCMInit::runModules(int phase)
{
    QString KCMINIT_PREFIX = QStringLiteral("kcminit_");
    for (const KService::Ptr &service : qAsConst(m_list)) {
        const QVariant tmp = service->property(QStringLiteral("X-KDE-Init-Library"), QVariant::String);
        QString library;
        if (tmp.isValid()) {
            library = tmp.toString();
            if (!library.startsWith(KCMINIT_PREFIX))
                library = KCMINIT_PREFIX + library;
        } else {
            library = service->library();
        }

        if (library.isEmpty()) {
            qWarning() << Q_FUNC_INFO << "library is empty, skipping";
            continue; // Skip
        }

        // see ksmserver's README for the description of the phases
        const QVariant vphase = service->property(QStringLiteral("X-KDE-Init-Phase"), QVariant::Int);

        int libphase = 1;
        if (vphase.isValid())
            libphase = vphase.toInt();

        if (libphase > 1) {
            libphase = 1;
        }

        if (phase != -1 && libphase != phase)
            continue;

        // try to load the library
        if (!m_alreadyInitialized.contains(library)) {
            runModule(library, service);
            m_alreadyInitialized.insert(library);
        }
    }
}

KCMInit::KCMInit(const QCommandLineParser &args)
{
    QString arg;
    if (args.positionalArguments().size() == 1) {
        arg = args.positionalArguments().first();
    }

    if (args.isSet(QStringLiteral("list"))) {
        m_list = KServiceTypeTrader::self()->query(QStringLiteral("KCModuleInit"));

        for (const KService::Ptr &service : qAsConst(m_list)) {
            if (service->library().isEmpty())
                continue; // Skip
            printf("%s\n", QFile::encodeName(service->desktopEntryName()).data());
        }
        return;
    }

    if (!arg.isEmpty()) {
        QString module = arg;
        if (!module.endsWith(QLatin1String(".desktop")))
            module += QLatin1String(".desktop");

        KService::Ptr serv = KService::serviceByStorageId(module);
        if (!serv || serv->library().isEmpty()) {
            qCritical() << i18n("Module %1 not found", module);
            return;
        } else {
            m_list.append(serv);
        }
    } else {
        // locate the desktop files
        m_list = KServiceTypeTrader::self()->query(QStringLiteral("KCModuleInit"));
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

extern "C" Q_DECL_EXPORT int kdemain(int argc, char *argv[])
{
    // kdeinit waits for kcminit to finish, but during KDE startup
    // only important kcm's are started very early in the login process,
    // the rest is delayed, so fork and make parent return after the initial phase
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
