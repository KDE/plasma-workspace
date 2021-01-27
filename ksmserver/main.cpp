/*****************************************************************
ksmserver - the KDE session management server

Copyright 2000 Matthias Ettrich <ettrich@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/
#include <config-ksmserver.h>
#include <config-workspace.h>
#include <errno.h>
#include <fcntl.h>
#include <fixx11h.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KMessageBox>

#include "server.h"
#include <KLocalizedString>
#include <KSharedConfig>
#include <QX11Info>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kdbusservice.h>
#include <kmanagerselection.h>
#include <ksmserver_debug.h>
#include <kwindowsystem.h>

#include <QApplication>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <X11/extensions/Xrender.h>

static const char version[] = "0.4";
static const char description[] = I18N_NOOP("The reliable Plasma session manager that talks the standard X11R6 \nsession management protocol (XSMP).");

Display *dpy = nullptr;
Colormap colormap = 0;
Visual *visual = nullptr;

extern KSMServer *the_server;

void IoErrorHandler(IceConn iceConn)
{
    the_server->ioError(iceConn);
}

bool writeTest(QByteArray path)
{
    path += "/XXXXXX";
    int fd = mkstemp(path.data());
    if (fd == -1)
        return false;
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

void checkComposite()
{
    if (qgetenv("KDE_SKIP_ARGB_VISUALS") == "1")
        return;
    // thanks to zack rusin and frederik for pointing me in the right direction
    // for the following bits of X11 code
    dpy = XOpenDisplay(nullptr); // open default display
    if (!dpy) {
        qCCritical(KSMSERVER) << "Cannot connect to the X server";
        return;
    }

    int screen = DefaultScreen(dpy);
    int eventBase, errorBase;

    if (XRenderQueryExtension(dpy, &eventBase, &errorBase)) {
        int nvi;
        XVisualInfo templ;
        templ.screen = screen;
        templ.depth = 32;
        templ.c_class = TrueColor;
        XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask | VisualDepthMask | VisualClassMask, &templ, &nvi);
        for (int i = 0; i < nvi; ++i) {
            XRenderPictFormat *format = XRenderFindVisualFormat(dpy, xvi[i].visual);
            if (format->type == PictTypeDirect && format->direct.alphaMask) {
                visual = xvi[i].visual;
                colormap = XCreateColormap(dpy, RootWindow(dpy, screen), visual, AllocNone);

                XFree(xvi);
                return;
            }
        }

        XFree(xvi);
    }
    XCloseDisplay(dpy);
    dpy = nullptr;
}

void sanity_check(int argc, char *argv[])
{
    QString msg;
    QByteArray path = qgetenv("HOME");
    const QByteArray readOnly = qgetenv("KDE_HOME_READONLY");
    if (path.isEmpty()) {
        msg = i18n("$HOME not set!");
    }
    if (msg.isEmpty() && access(path.data(), W_OK)) {
        if (errno == ENOENT)
            msg = i18n("$HOME directory (%1) does not exist.", QFile::decodeName(path));
        else if (readOnly.isEmpty())
            msg = i18n("No write access to $HOME directory (%1).", QFile::decodeName(path));
    }
    if (msg.isEmpty() && access(path.data(), R_OK)) {
        if (errno == ENOENT)
            msg = i18n("$HOME directory (%1) does not exist.", QFile::decodeName(path));
        else
            msg = i18n("No read access to $HOME directory (%1).", QFile::decodeName(path));
    }
    if (msg.isEmpty() && readOnly.isEmpty() && !writeTest(path)) {
        if (errno == ENOSPC)
            msg = i18n("$HOME directory (%1) is out of disk space.", QFile::decodeName(path));
        else
            msg = i18n(
                "Writing to the $HOME directory (%2) failed with "
                "the error '%1'",
                QString::fromLocal8Bit(strerror(errno)),
                QFile::decodeName(path));
    }
    if (msg.isEmpty()) {
        path = getenv("ICEAUTHORITY");
        if (path.isEmpty()) {
            path = qgetenv("HOME");
            path += "/.ICEauthority";
        }

        if (access(path.data(), W_OK) && (errno != ENOENT))
            msg = i18n("No write access to '%1'.", QFile::decodeName(path));
        else if (access(path.data(), R_OK) && (errno != ENOENT))
            msg = i18n("No read access to '%1'.", QFile::decodeName(path));
    }
    if (msg.isEmpty()) {
        path = getenv("KDETMP");
        if (path.isEmpty())
            path = "/tmp";
        if (!writeTest(path)) {
            if (errno == ENOSPC)
                msg = i18n("Temp directory (%1) is out of disk space.", QFile::decodeName(path));
            else
                msg = i18n(
                    "Writing to the temp directory (%2) failed with\n    "
                    "the error '%1'",
                    QString::fromLocal8Bit(strerror(errno)),
                    QFile::decodeName(path));
        }
    }
    if (msg.isEmpty() && (path != "/tmp")) {
        path = "/tmp";
        if (!writeTest(path)) {
            if (errno == ENOSPC)
                msg = i18n("Temp directory (%1) is out of disk space.", QFile::decodeName(path));
            else
                msg = i18n(
                    "Writing to the temp directory (%2) failed with\n    "
                    "the error '%1'",
                    QString::fromLocal8Bit(strerror(errno)),
                    QFile::decodeName(path));
        }
    }
    if (msg.isEmpty()) {
        path += "/.ICE-unix";
        if (access(path.data(), W_OK) && (errno != ENOENT))
            msg = i18n("No write access to '%1'.", QFile::decodeName(path));
        else if (access(path.data(), R_OK) && (errno != ENOENT))
            msg = i18n("No read access to '%1'.", QFile::decodeName(path));
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

        QApplication a(argc, argv);
        const QString qmsg = msg_pre + msg + msg_post;
        KMessageBox::error(nullptr, qmsg, i18n("Plasma Workspace installation problem!"));
        exit(255);
    }
}

int main(int argc, char *argv[])
{
    sanity_check(argc, argv);

    putenv((char *)"SESSION_MANAGER=");
    checkComposite();

    // force xcb QPA plugin as ksmserver is very X11 specific
    const QByteArray origQpaPlatform = qgetenv("QT_QPA_PLATFORM");
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("xcb"));

    QQuickWindow::setDefaultAlphaBuffer(true);
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
    QApplication *a = new QApplication(argc, argv);

    // now the QPA platform is set, unset variable again to not launch apps with incorrect environment
    if (origQpaPlatform.isEmpty()) {
        qunsetenv("QT_QPA_PLATFORM");
    } else {
        qputenv("QT_QPA_PLATFORM", origQpaPlatform);
    }

    QApplication::setApplicationName(QStringLiteral("ksmserver"));
    QApplication::setApplicationVersion(QString::fromLatin1(version));
    QApplication::setOrganizationDomain(QStringLiteral("kde.org"));

    fcntl(ConnectionNumber(QX11Info::display()), F_SETFD, 1);

    a->setQuitOnLastWindowClosed(false); // #169486

    QCommandLineParser parser;
    parser.setApplicationDescription(i18n(description));
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

    KSMServer *server = new KSMServer(flags);

    // for the KDE-already-running check in startkde
    KSelectionOwner kde_running("_KDE_RUNNING", 0);
    kde_running.claim(false);

    IceSetIOErrorHandler(IoErrorHandler);

    KConfigGroup config(KSharedConfig::openConfig(), "General");

    QString loginMode = config.readEntry("loginMode", "restorePreviousLogout");

    if (parser.isSet(restoreOption))
        server->restoreSession(QStringLiteral(SESSION_BY_USER));
    else if (loginMode == QLatin1String("restorePreviousLogout"))
        server->restoreSession(QStringLiteral(SESSION_PREVIOUS_LOGOUT));
    else if (loginMode == QLatin1String("restoreSavedSession"))
        server->restoreSession(QStringLiteral(SESSION_BY_USER));
    else
        server->startDefaultSession();

    KDBusService service(KDBusService::Unique);

    server->setupShortcuts();
    int ret = a->exec();
    kde_running.release(); // needs to be done before QApplication destruction
    delete a;
    return ret;
}
