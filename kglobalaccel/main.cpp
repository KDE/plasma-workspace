/*
  This file is part of the KDE project

  Copyright (c) 2007 Andreas Hartmetz <ahartmetz@gmail.com>
  Copyright (c) 2007 Michael Jansen <kde@michael-jansen.biz>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
  */

#include "kglobalacceld.h"

#include <kcrash.h>
#include <KAboutData>
#include <KLocalizedString>
#include <KDBusService>
#include <QDebug>
#include <QApplication>

#include <signal.h>

static bool isEnabled()
{
    // TODO: Check if kglobalaccel can be disabled
    return true;
}


static void sighandler(int /*sig*/)
{
    if (qApp)
       qApp->quit();
}


extern "C" Q_DECL_EXPORT int kdemain(int argc, char **argv)
{
    // Disable Session Management the right way (C)
    //
    // ksmserver has global shortcuts. disableSessionManagement() does not prevent Qt from
    // registering the app with the session manager. We remove the address to make sure we do not
    // get a hang on kglobalaccel restart (kglobalaccel tries to register with ksmserver,
    // ksmserver tries to register with kglobalaccel).
    unsetenv( "SESSION_MANAGER" );

    QApplication app(argc, argv);
    KAboutData aboutdata(
            "kglobalaccel",
            QString(),
            i18n("KDE Global Shortcuts Service"),
            "0.2",
            i18n("KDE Global Shortcuts Service"),
            KAboutData::License_LGPL,
            i18n("(C) 2007-2009  Andreas Hartmetz, Michael Jansen"));
    aboutdata.addAuthor(i18n("Andreas Hartmetz"),i18n("Maintainer"),"ahartmetz@gmail.com");
    aboutdata.addAuthor(i18n("Michael Jansen"),i18n("Maintainer"),"kde@michael-jansen.biz");

    aboutdata.setProgramIconName("kglobalaccel");
    KAboutData::setApplicationData(aboutdata);

    // check if kglobalaccel is disabled
    if (!isEnabled()) {
        qDebug() << "kglobalaccel is disabled!";
        return 0;
    }

    KDBusService service(KDBusService::Unique);

    app.setQuitOnLastWindowClosed( false );

    // Stop gracefully
    ::signal(SIGINT, sighandler);
    ::signal(SIGTERM, sighandler);
    ::signal(SIGHUP, sighandler);

    // Restart on a crash
    KCrash::setFlags(KCrash::AutoRestart);

    KGlobalAccelD globalaccel;
    if (!globalaccel.init()) {
        return -1;
    }

    return app.exec();
}
