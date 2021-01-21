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

#include <KAboutData>
#include <KConfigDialogManager>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QSessionManager>

#include "klipper.h"
#include "tray.h"

extern "C" int Q_DECL_EXPORT kdemain(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("klipper");

    KAboutData aboutData(QStringLiteral("klipper"),
                         i18n("Klipper"),
                         QStringLiteral(KLIPPER_VERSION_STRING),
                         i18n("KDE cut & paste history utility"),
                         KAboutLicense::GPL,
                         i18n("(c) 1998, Andrew Stanley-Jones\n"
                              "1998-2002, Carsten Pfeiffer\n"
                              "2001, Patrick Dubroy"));
    aboutData.addAuthor(i18n("Carsten Pfeiffer"), i18n("Author"), QStringLiteral("pfeiffer@kde.org"));

    aboutData.addAuthor(i18n("Andrew Stanley-Jones"), i18n("Original Author"), QStringLiteral("asj@cban.com"));

    aboutData.addAuthor(i18n("Patrick Dubroy"), i18n("Contributor"), QStringLiteral("patrickdu@corel.com"));

    aboutData.addAuthor(i18n("Luboš Luňák"), i18n("Bugfixes and optimizations"), QStringLiteral("l.lunak@kde.org"));

    aboutData.addAuthor(i18n("Esben Mose Hansen"), i18n("Previous Maintainer"), QStringLiteral("kde@mosehansen.dk"));

    aboutData.addAuthor(i18n("Martin Gräßlin"), i18n("Maintainer"), QStringLiteral("mgraesslin@kde.org"));

    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    KAboutData::setApplicationData(aboutData);

    QGuiApplication::setFallbackSessionManagementEnabled(false);

    auto disableSessionManagement = [](QSessionManager &sm) {
        sm.setRestartHint(QSessionManager::RestartNever);
    };
    QObject::connect(&app, &QGuiApplication::commitDataRequest, disableSessionManagement);
    QObject::connect(&app, &QGuiApplication::saveStateRequest, disableSessionManagement);
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDBusService service(KDBusService::Unique);

    // make KConfigDialog "know" when our actions page is changed
    KConfigDialogManager::changedMap()->insert(QStringLiteral("ActionsTreeWidget"), SIGNAL(changed()));

    KlipperTray klipper;
    return app.exec();
}
