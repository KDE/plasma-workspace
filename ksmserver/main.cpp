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
#include <fixx11h.h>
#include <config-workspace.h>
#include <config-ksmserver.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <KMessageBox>
#include <QtDBus/QtDBus>

#include <k4aboutdata.h>
#include <kcmdlineargs.h>
#include <kconfiggroup.h>
#include <kdbusservice.h>
#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kmanagerselection.h>
#include <kwindowsystem.h>
#include "server.h"
#include <QX11Info>

#include <QApplication>
#include <X11/extensions/Xrender.h>

static const char version[] = "0.4";
static const char description[] = I18N_NOOP( "The reliable KDE session manager that talks the standard X11R6 \nsession management protocol (XSMP)." );

Display* dpy = 0;
Colormap colormap = 0;
Visual *visual = 0;

extern KSMServer* the_server;

void IoErrorHandler ( IceConn iceConn)
{
    the_server->ioError( iceConn );
}

bool writeTest(QByteArray path)
{
   path += "/XXXXXX";
   int fd = mkstemp(path.data());
   if (fd == -1)
      return false;
   if (write(fd, "Hello World\n", 12) == -1)
   {
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
    if( qgetenv( "KDE_SKIP_ARGB_VISUALS" ) == "1" )
        return;
    // thanks to zack rusin and frederik for pointing me in the right direction
    // for the following bits of X11 code
    dpy = XOpenDisplay(0); // open default display
    if (!dpy)
    {
        kError() << "Cannot connect to the X server";
        return;
    }

    int screen = DefaultScreen(dpy);
    int eventBase, errorBase;

    if (XRenderQueryExtension(dpy, &eventBase, &errorBase))
    {
        int nvi;
        XVisualInfo templ;
        templ.screen  = screen;
        templ.depth   = 32;
        templ.c_class = TrueColor;
        XVisualInfo *xvi = XGetVisualInfo(dpy, VisualScreenMask |
                                                VisualDepthMask |
                                                VisualClassMask,
                                            &templ, &nvi);
        for (int i = 0; i < nvi; ++i)
        {
            XRenderPictFormat *format = XRenderFindVisualFormat(dpy,
                                                                xvi[i].visual);
            if (format->type == PictTypeDirect && format->direct.alphaMask)
            {
                visual = xvi[i].visual;
                colormap = XCreateColormap(dpy, RootWindow(dpy, screen),
                                            visual, AllocNone);

                XFree(xvi);
                return;
            }
        }

        XFree(xvi);

    }
    XCloseDisplay( dpy );
    dpy = NULL;
}

void sanity_check( int argc, char* argv[] )
{
    QString msg;
    QByteArray path = getenv("HOME");
    QByteArray readOnly = getenv("KDE_HOME_READONLY");
    if (path.isEmpty())
    {
        msg = QLatin1String("$HOME not set!");
    }
    if (msg.isEmpty() && access(path.data(), W_OK))
    {
        if (errno == ENOENT)
            msg = QStringLiteral("$HOME directory (%1) does not exist.");
        else if (readOnly.isEmpty())
            msg = QStringLiteral("No write access to $HOME directory (%1).");
    }
    if (msg.isEmpty() && access(path.data(), R_OK))
    {
        if (errno == ENOENT)
            msg = QStringLiteral("$HOME directory (%1) does not exist.");
        else
            msg = QStringLiteral("No read access to $HOME directory (%1).");
    }
    if (msg.isEmpty() && readOnly.isEmpty() && !writeTest(path))
    {
        if (errno == ENOSPC)
            msg = QStringLiteral("$HOME directory (%1) is out of disk space.");
        else
            msg = QStringLiteral("Writing to the $HOME directory (%1) failed with\n    "
                "the error '")+QString::fromLocal8Bit(strerror(errno))+QStringLiteral("'");
    }
    if (msg.isEmpty())
    {
        path = getenv("ICEAUTHORITY");
        if (path.isEmpty())
        {
            path = getenv("HOME");
            path += "/.ICEauthority";
        }
    
        if (access(path.data(), W_OK) && (errno != ENOENT))
            msg = QStringLiteral("No write access to '%1'.");
        else if (access(path.data(), R_OK) && (errno != ENOENT))
            msg = QStringLiteral("No read access to '%1'.");
    }
    if (msg.isEmpty())
    {
        path = getenv("KDETMP");
        if (path.isEmpty())
            path = "/tmp";
        if (!writeTest(path))
        {
            if (errno == ENOSPC)
            msg = QStringLiteral("Temp directory (%1) is out of disk space.");
            else
            msg = QStringLiteral("Writing to the temp directory (%1) failed with\n    "
                    "the error '")+QString::fromLocal8Bit(strerror(errno))+QStringLiteral("'");
        }
    }
    if (msg.isEmpty() && (path != "/tmp"))
    {
        path = "/tmp";
        if (!writeTest(path))
        {
            if (errno == ENOSPC)
            msg = QStringLiteral("Temp directory (%1) is out of disk space.");
            else
            msg = QStringLiteral("Writing to the temp directory (%1) failed with\n    "
                    "the error '")+QString::fromLocal8Bit(strerror(errno))+QStringLiteral("'");
        }
    }
    if (msg.isEmpty())
    {
        path += "/.ICE-unix";
        if (access(path.data(), W_OK) && (errno != ENOENT))
            msg = QStringLiteral("No write access to '%1'.");
        else if (access(path.data(), R_OK) && (errno != ENOENT))
            msg = QStringLiteral("No read access to '%1'.");
    }
    if (!msg.isEmpty())
    {
        const char *msg_pre =
                "The following installation problem was detected\n"
                "while trying to start KDE:"
                "\n\n    ";
        const char *msg_post = "\n\nKDE is unable to start.\n";
        fputs(msg_pre, stderr);
        fprintf(stderr, "%s", qPrintable(msg.arg(QFile::decodeName(path))));
        fputs(msg_post, stderr);

        QApplication a(argc, argv);
        QString qmsg = QString::fromLatin1(msg_pre) +
                       msg.arg(QFile::decodeName(path)) +
                       QString::fromLatin1(msg_post);
        KMessageBox::error(0, qmsg, QStringLiteral("KDE Workspace installation problem!"));
        exit(255);
    }
}

extern "C" Q_DECL_EXPORT int kdemain( int argc, char* argv[] )
{
    sanity_check(argc, argv);

    putenv((char*)"SESSION_MANAGER=");
    checkComposite();

    QApplication *a = new QApplication(argc, argv);

    QApplication::setApplicationName( QStringLiteral( "ksmserver") );
    QApplication::setApplicationVersion( QString::fromLatin1( version ) );
    QApplication::setOrganizationDomain( QStringLiteral( "kde.org") );

    fcntl(ConnectionNumber(QX11Info::display()), F_SETFD, 1);

    a->setQuitOnLastWindowClosed(false); // #169486

    QCommandLineParser parser;
    parser.setApplicationDescription(QString::fromLatin1(description));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption restoreOption(QStringList() << QStringLiteral("r") << QStringLiteral("restore"),
                                     i18n("Restores the saved user session if available"));
    parser.addOption(restoreOption);

    QCommandLineOption wmOption(QStringList() << QStringLiteral("w") << QStringLiteral("windowmanager"),
                                i18n("Starts <wm> in case no other window manager is \nparticipating in the session. Default is 'kwin'"),
                                i18n("wm"));
    parser.addOption(wmOption);

    QCommandLineOption nolocalOption(QStringLiteral("nolocal"),
                                     i18n("Also allow remote connections"));
    parser.addOption(nolocalOption);

#if COMPILE_SCREEN_LOCKER
    QCommandLineOption lockscreenOption(QStringLiteral("lockscreen"),
                                        i18n("Starts the session in locked mode"));
    parser.addOption(lockscreenOption);
#endif

    parser.process(*a);

//TODO: should we still use this?
//    if( !QDBusConnection::sessionBus().interface()->
//            registerService( QStringLiteral( "org.kde.ksmserver" ),
//                             QDBusConnectionInterface::DontQueueService ) )
//    {
//        qWarning("Could not register with D-BUS. Aborting.");
//        return 1;
//    }

    QString wm = parser.value(wmOption);

    bool only_local = !parser.isSet(nolocalOption);
#ifndef HAVE__ICETRANSNOLISTEN
    /* this seems strange, but the default is only_local, so if !only_local
     * the option --nolocal was given, and we warn (the option --nolocal
     * does nothing on this platform, as here the default is reversed)
     */
    if (!only_local) {
        qWarning("--nolocal is not supported on your platform. Sorry.");
    }
    only_local = false;
#endif

#if COMPILE_SCREEN_LOCKER
    KSMServer *server = new KSMServer( wm, only_local, parser.isSet(lockscreenOption ) );
#else
    KSMServer *server = new KSMServer( wm, only_local );
#endif
    
    // for the KDE-already-running check in startkde
    KSelectionOwner kde_running( "_KDE_RUNNING", 0 );
    kde_running.claim( false );

    IceSetIOErrorHandler( IoErrorHandler );

    KConfigGroup config(KSharedConfig::openConfig(), "General");

    int realScreenCount = ScreenCount( QX11Info::display() );
    bool screenCountChanged =
         ( config.readEntry( "screenCount", realScreenCount ) != realScreenCount );

    QString loginMode = config.readEntry( "loginMode", "restorePreviousLogout" );

    if ( parser.isSet( restoreOption ) && ! screenCountChanged )
        server->restoreSession( QStringLiteral( SESSION_BY_USER ) );
    else if ( loginMode == QStringLiteral( "default" ) || screenCountChanged )
        server->startDefaultSession();
    else if ( loginMode == QStringLiteral( "restorePreviousLogout" ) )
        server->restoreSession( QStringLiteral( SESSION_PREVIOUS_LOGOUT ) );
    else if ( loginMode == QStringLiteral( "restoreSavedSession" ) )
        server->restoreSession( QStringLiteral( SESSION_BY_USER ) );
    else
        server->startDefaultSession();

    KDBusService service(KDBusService::Unique);

    int ret = a->exec();
    kde_running.release(); // needs to be done before QApplication destruction
    delete a;
    return ret;
}
