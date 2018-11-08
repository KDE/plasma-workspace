#include "shutdown.h"
#include "shutdownadaptor.h"

#include <QDBusConnection>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

#include <kdisplaymanager.h>

#include "server.h"
#include "ksmserver_debug.h"


Shutdown::Shutdown(QObject *parent):
    QObject(parent)
{
    new ShutdownAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Shutdown"), QStringLiteral("org.kde.Shutdown"), this);

    //registered as a new service name for easy moving to new process
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Shutdown"));

    connect(qApp, &QCoreApplication::aboutToQuit, this, &Shutdown::logoutComplete);
    connect(KSMServer::self(), &KSMServer::logoutCancelled, this, &Shutdown::logoutCancelled);
}

void Shutdown::logout()
{
    startLogout(KWorkSpace::ShutdownTypeNone);
}

void Shutdown::logoutAndShutdown()
{
    startLogout(KWorkSpace::ShutdownTypeHalt);
}

void Shutdown::logoutAndReboot()
{
    startLogout(KWorkSpace::ShutdownTypeReboot);
}

void Shutdown::startLogout(KWorkSpace::ShutdownType shutdownType)
{
    auto ksmserver = KSMServer::self();
    m_shutdownType = shutdownType;
    ksmserver->performLogout();
}

void Shutdown::logoutCancelled()
{
    m_shutdownType = KWorkSpace::ShutdownTypeNone;
}

void Shutdown::logoutComplete() {
    runShutdownScripts();
    KDisplayManager().shutdown( m_shutdownType, KWorkSpace::ShutdownModeDefault);
}

void Shutdown::runShutdownScripts()
{
    const QStringList shutdownFolders = QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation, QStringLiteral("plasma-workspace/shutdown"), QStandardPaths::LocateDirectory);
    foreach (const QString &shutDownFolder, shutdownFolders) {
        QDir dir(shutDownFolder);

        const QStringList entries = dir.entryList(QDir::Files);
        foreach (const QString &file, entries) {
            // Don't execute backup files
            if (!file.endsWith(QLatin1Char('~')) && !file.endsWith(QStringLiteral(".bak")) &&
                    (file[0] != QLatin1Char('%') || !file.endsWith(QLatin1Char('%'))) &&
                    (file[0] != QLatin1Char('#') || !file.endsWith(QLatin1Char('#'))))
            {
                const QString fullPath = dir.absolutePath() + QLatin1Char('/') + file;

                qCDebug(KSMSERVER) << "running shutdown script" << fullPath;
                QProcess::execute(fullPath, QStringList());
            }
        }
    }
}

