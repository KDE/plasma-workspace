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
#include <KLocalizedString>
#include <k4aboutdata.h>
#include <KConfigDialogManager>
#include <KDBusService>

#include <QApplication>
#include <QCommandLineParser>
#include <QSessionManager>

#include "tray.h"
#include "klipper.h"

extern "C" int Q_DECL_EXPORT kdemain(int argc, char *argv[])
{
  QCoreApplication::setApplicationName(i18n("Klipper"));
  QCoreApplication::setApplicationVersion(QStringLiteral(KLIPPER_VERSION_STRING));
  QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
  QApplication::setApplicationDisplayName(i18n("KDE cut & paste history utility"));
  Klipper::createAboutData();

  QApplication app(argc, argv);
  auto disableSessionManagement = [](QSessionManager &sm) {
      sm.setRestartHint(QSessionManager::RestartNever);
  };
  QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
  QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);
  app.setQuitOnLastWindowClosed( false );

  QCommandLineParser parser;
  parser.addHelpOption();
  parser.addVersionOption();
  parser.setApplicationDescription(QApplication::applicationDisplayName());
  parser.process(app);

  KDBusService service(KDBusService::Unique);

  // make KConfigDialog "know" when our actions page is changed
  KConfigDialogManager::changedMap()->insert("ActionsTreeWidget", SIGNAL(changed()));

  KlipperTray klipper;
  int ret = app.exec();
  Klipper::destroyAboutData();
  return ret;
}
