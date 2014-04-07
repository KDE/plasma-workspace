/* This file is part of the KDE project
   Copyright (c) 2004 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kio_remote.h"
#include "testremote.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kcmdlineargs.h>

#include <stdlib.h>

int main(int argc, char *argv[])
{
    //KApplication::disableAutoDcopRegistration();
    KCmdLineArgs::init(argc,argv,"testremote", 0, KLocalizedString(), 0);
    KApplication app;

    TestRemote test;
    test.setup();
    test.runAll();
    kDebug() << "All tests OK.";
    return 0; // success. The exit(1) in check() is what happens in case of failure.
}

void TestRemote::setup()
{

}

void TestRemote::runAll()
{

}

