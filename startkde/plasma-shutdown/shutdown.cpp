#include "shutdown.h"
#include "shutdownadaptor.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

#include "debug.h"
#include "ksmserver_interface.h"
#include "kwin_interface.h"
#include "sessionmanagementbackend.h"

Shutdown::Shutdown(QObject *parent)
    : QObject(parent)
{
    new ShutdownAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Shutdown"), QStringLiteral("org.kde.Shutdown"), this);
    QDBusConnection::sessionBus().registerService(QStringLiteral("org.kde.Shutdown"));
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
    m_shutdownType = shutdownType;

    OrgKdeKSMServerInterfaceInterface ksmserverIface(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
    ksmserverIface.setTimeout(
        INT32_MAX); // KSMServer closeSession can take a long time to reply, as apps may have prompts.  Value corresponds to DBUS_TIMEOUT_INFINITE

    auto closeSessionReply = ksmserverIface.closeSession();
    auto watcher = new QDBusPendingCallWatcher(closeSessionReply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [closeSessionReply, watcher, this]() {
        watcher->deleteLater();
        if (closeSessionReply.isError()) {
            qCWarning(PLASMA_SESSION) << "ksmserver failed to complete logout";
            logoutCancelled();
            return;
        }
        if (closeSessionReply.value()) {
            logoutComplete();
        } else {
            logoutCancelled();
        }
    });
}

void Shutdown::logoutCancelled()
{
    m_shutdownType = KWorkSpace::ShutdownTypeNone;
    qApp->quit();
}

void Shutdown::logoutComplete()
{
    runShutdownScripts();

    auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.systemd1"),
                                              QStringLiteral("/org/freedesktop/systemd1"),
                                              QStringLiteral("org.freedesktop.systemd1.Manager"),
                                              QStringLiteral("StopUnit"));
    msg << QStringLiteral("graphical-session.target") << QStringLiteral("fail");
    QDBusReply<QDBusObjectPath> reply = QDBusConnection::sessionBus().call(msg);

    if (!reply.isValid()) {
        OrgKdeKWinSessionInterface kwinInterface(QStringLiteral("org.kde.KWin"), QStringLiteral("/Session"), QDBusConnection::sessionBus());
        QDBusPendingReply<> reply = kwinInterface.quit();
        reply.waitForFinished();
    }

    if (m_shutdownType == KWorkSpace::ShutdownTypeHalt) {
        SessionBackend::self()->shutdown();
    } else if (m_shutdownType == KWorkSpace::ShutdownTypeReboot) {
        SessionBackend::self()->reboot();
    } else { // logout
        qApp->quit();
    }
}

void Shutdown::runShutdownScripts()
{
    const QStringList shutdownFolders =
        QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation, QStringLiteral("plasma-workspace/shutdown"), QStandardPaths::LocateDirectory);
    for (const QString &shutDownFolder : shutdownFolders) {
        QDir dir(shutDownFolder);

        const QStringList entries = dir.entryList(QDir::Files);
        for (const QString &file : entries) {
            // Don't execute backup files
            if (!file.endsWith(QLatin1Char('~')) && !file.endsWith(QLatin1String(".bak")) && (file[0] != QLatin1Char('%') || !file.endsWith(QLatin1Char('%')))
                && (file[0] != QLatin1Char('#') || !file.endsWith(QLatin1Char('#')))) {
                const QString fullPath = dir.absolutePath() + QLatin1Char('/') + file;

                qCDebug(PLASMA_SESSION) << "running shutdown script" << fullPath;
                QProcess::execute(fullPath, QStringList());
            }
        }
    }
}
