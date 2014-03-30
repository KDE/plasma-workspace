/* This file is part of the KDE project
   Copyright (C) Andrew Stanley-Jones <asj@cban.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>

#include <kdemacros.h>
#include <KLocale>
#include <KCmdLineArgs>
#include <k4aboutdata.h>
#include <KUniqueApplication>
#include <KConfigDialogManager>

#include "tray.h"
#include "klipper.h"

extern "C" int Q_DECL_EXPORT kdemain(int argc, char *argv[])
{
  Klipper::createAboutData();
  KCmdLineArgs::init( argc, argv, Klipper::aboutData());
  KUniqueApplication::addCmdLineOptions();

  if (!KUniqueApplication::start()) {
       fprintf(stderr, "Klipper is already running! Check it in the system tray in the panel.\n");
       exit(0);
  }
  KUniqueApplication app;
  app.disableSessionManagement();
  app.setQuitOnLastWindowClosed( false );

  // make KConfigDialog "know" when our actions page is changed
  KConfigDialogManager::changedMap()->insert("ActionsTreeWidget", SIGNAL(changed()));

  KlipperTray klipper;
  int ret = app.exec();
  Klipper::destroyAboutData();
  return ret;
}
