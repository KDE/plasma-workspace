/*
    ksmserver - the KDE session management server

    SPDX-FileCopyrightText: 2000 Matthias Ettrich <ettrich@kde.org>

    SPDX-License-Identifier: MIT
*/

#include <cerrno>
#include <config-ksmserver.h>
#include <config-workspace.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fixx11h.h>
#include <unistd.h>

#include "server.h"
#include <KAboutData>
#include <KCrash>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRuntimePlatform>
#include <KSelectionOwner>
#include <KSharedConfig>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdbusservice.h>
#include <ksmserver_debug.h>
#include <kwindowsystem.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QFile>

void IoErrorHandler(IceConn iceConn)
{
    KSMServer::self()->ioError(iceConn);
}

bool writeTest(QByteArray path)
{
    path += "/XXXXXX";
    int fd = mkstemp(path.data());
    if (fd == -1) {
        return false;
    }
    if (write(fd, "Hello World\n", 12) == -1) {
        int save_errno = errno;
        close(fd);
        unlink(path.data());
        errno = save_errno;
        return false;
    }
    close(fd);
    unlink(path.data());
    return true;
}

void sanity_check(int argc, char *argv[])
{
    QString msg;
    QByteArray path;
    if (msg.isEmpty()) {
        path = getenv("ICEAUTHORITY");
        if (path.isEmpty()) {
            path = qgetenv("HOME");
            path += "/.ICEauthority";
        }

        if (access(path.data(), W_OK) && (errno != ENOENT)) {
            msg = i18n("No write access to '%1'.", QFile::decodeName(path));
        } else if (access(path.data(), R_OK) && (errno != ENOENT)) {
            msg = i18n("No read access to '%1'.", QFile::decodeName(path));
        }
    }
    if (msg.isEmpty()) {
        path = "/tmp/.ICE-unix";
        if (access(path.data(), W_OK) && (errno != ENOENT)) {
            msg = i18n("No write access to '%1'.", QFile::decodeName(path));
        } else if (access(path.data(), R_OK) && (errno != ENOENT)) {
            msg = i18n("No read access to '%1'.", QFile::decodeName(path));
        }
    }
    if (!msg.isEmpty()) {
        const QString msg_pre = i18n(
                                    "The following installation problem was detected\n"
                                    "while trying to start Plasma:")
            + QStringLiteral("\n\n    ");
        const QString msg_post = i18n("\n\nPlasma is unable to start.\n");
        fputs(msg_pre.toUtf8().constData(), stderr);
        fprintf(stderr, "%s", msg.toUtf8().constData());
        fputs(msg_post.toUtf8().constData(), stderr);

        exit(255);
    }
}

int main(int argc, char *argv[])
{
    sanity_check(argc, argv);

    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager);

    // force xcb QPA plugin as ksmserver is very X11 specific
    const QByteArray origQpaPlatform = qgetenv("QT_QPA_PLATFORM");
    qputenv("QT_QPA_PLATFORM", QByteArrayView("xcb"));

    QCoreApplication::setQuitLockEnabled(false);
    auto a = new QGuiApplication(argc, argv);

    // now the QPA platform is set, unset variable again to not launch apps with incorrect environment
    if (origQpaPlatform.isEmpty()) {
        qunsetenv("QT_QPA_PLATFORM");
    } else {
        qputenv("QT_QPA_PLATFORM", origQpaPlatform);
    }

    KAboutData about(QStringLiteral("ksmserver"), QString(), QStringLiteral(WORKSPACE_VERSION_STRING));
    KAboutData::setApplicationData(about);

    KCrash::initialize();

    fcntl(ConnectionNumber(a->nativeInterface<QNativeInterface::QX11Application>()->display()), F_SETFD, 1);

    a->setQuitOnLastWindowClosed(false); // #169486

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("The reliable Plasma session manager that talks the standard X11R6 \nsession management protocol (XSMP)."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption restoreOption(QStringList() << QStringLiteral("r") << QStringLiteral("restore"), i18n("Restores the saved user session if available"));
    parser.addOption(restoreOption);

    QCommandLineOption nolocalOption(QStringLiteral("nolocal"), i18n("Also allow remote connections"));
    parser.addOption(nolocalOption);

    QCommandLineOption lockscreenOption(QStringLiteral("lockscreen"), i18n("Starts the session in locked mode"));
    parser.addOption(lockscreenOption);

    QCommandLineOption noLockscreenOption(QStringLiteral("no-lockscreen"),
                                          i18n("Starts without lock screen support. Only needed if other component provides the lock screen."));
    parser.addOption(noLockscreenOption);

    parser.process(*a);

    bool only_local = !parser.isSet(nolocalOption);
#ifndef HAVE__ICETRANSNOLISTEN
    /* this seems strange, but the default is only_local, so if !only_local
     * the option --nolocal was given, and we warn (the option --nolocal
     * does nothing on this platform, as here the default is reversed)
     */
    if (!only_local) {
        qCWarning(KSMSERVER, "--nolocal is not supported on your platform. Sorry.");
    }
    only_local = false;
#endif

    KSMServer::InitFlags flags = KSMServer::InitFlag::None;
    if (only_local) {
        flags |= KSMServer::InitFlag::OnlyLocal;
    }
    if (parser.isSet(lockscreenOption)) {
        flags |= KSMServer::InitFlag::ImmediateLockScreen;
    }
    if (parser.isSet(noLockscreenOption)) {
        flags |= KSMServer::InitFlag::NoLockScreen;
    }

    // we use the session_type here as ksmserver is already forced as X above
    // in wayland, kwin manages the lock screen
    if (qgetenv("XDG_SESSION_TYPE") == QByteArrayLiteral("wayland")) {
        flags |= KSMServer::InitFlag::NoLockScreen;
    }

    auto server = new KSMServer(flags);

    // for the KDE-already-running check in startkde
    KSelectionOwner kde_running("_KDE_RUNNING", 0);
    kde_running.claim(false);

    IceSetIOErrorHandler(IoErrorHandler);

    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("General"));

    QString loginMode = config.readEntry("loginMode", "restorePreviousLogout");

    // we don't need session restoring in Plasma Mobile
    if (KRuntimePlatform::runtimePlatform().contains(u"phone")) {
        loginMode = QStringLiteral("emptySession");
    }

    if (parser.isSet(restoreOption)) {
        server->setRestoreSession(SESSION_BY_USER);
    } else if (loginMode == QLatin1String("restorePreviousLogout")) {
        server->setRestoreSession(SESSION_PREVIOUS_LOGOUT);
    } else if (loginMode == QLatin1String("restoreSavedSession")) {
        server->setRestoreSession(SESSION_BY_USER);
    } else {
        server->startDefaultSession();
    }

    KDBusService service(KDBusService::Unique);

    int ret = a->exec();
    kde_running.release(); // needs to be done before QGuiApplication destruction
    delete a;
    return ret;
}
