/*
    SPDX-FileCopyrightText: Andrew Stanley-Jones <asj@cban.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <stdio.h>
#include <stdlib.h>

#include <KAboutData>
#include <KDBusService>
#include <KLocalizedString>

#include <QApplication>
#include <QCommandLineParser>
#include <QSessionManager>

#include "klipper.h"
#include "tray.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("klipper");

    KAboutData aboutData(QStringLiteral("klipper"),
                         i18n("Klipper"),
                         QStringLiteral(KLIPPER_VERSION_STRING),
                         i18n("Plasma cut & paste history utility"),
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QGuiApplication::setFallbackSessionManagementEnabled(false);
#endif

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

    KlipperTray klipper;
    return app.exec();
}
