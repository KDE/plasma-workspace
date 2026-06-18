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
#include <KRuntimePlatform>
#include <KSharedConfig>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdbusservice.h>
#include <ksmserver_debug.h>

#include <QCommandLineParser>
#include <QCoreApplication>
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

void sanity_check(int /*argc*/, char * /*argv*/[])
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

    QCoreApplication::setQuitLockEnabled(false);
    auto a = new QCoreApplication(argc, argv);

    KAboutData about(QStringLiteral("ksmserver"), QString(), QStringLiteral(WORKSPACE_VERSION_STRING));
    KAboutData::setApplicationData(about);

    KCrash::initialize();

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n("The reliable Plasma session manager that talks the standard X11R6 \nsession management protocol (XSMP)."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption restoreOption(QStringList() << QStringLiteral("r") << QStringLiteral("restore"), i18n("Restores the saved user session if available"));
    parser.addOption(restoreOption);

    QCommandLineOption nolocalOption(QStringLiteral("nolocal"), i18n("Also allow remote connections"));
    parser.addOption(nolocalOption);

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

    auto server = new KSMServer(flags);

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
    delete a;
    return ret;
}
