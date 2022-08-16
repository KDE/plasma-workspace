/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <QApplication>
#include <QDBusConnection>

#include <KAboutData>
#include <KLocalizedString>

#include <kworkspace.h>

#include "ksystemactivitydialog.h"

int main(int argc, char **argv)
{
    KWorkSpace::detectPlatform(argc, argv);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("systemmonitor");

    KAboutData about(QStringLiteral("systemmonitor"), i18n("System Activity"));
    KAboutData::setApplicationData(about);

    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService(QStringLiteral("org.kde.systemmonitor"))) {
        return 0;
    }

    KSystemActivityDialog *dialog = new KSystemActivityDialog;
    dialog->show();

    return app.exec();
}
