/*
    SPDX-FileCopyrightText: 2014 Vishesh Handa <me@vhanda.in>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include <KLocalizedString>
#include <QApplication>
#include <QDBusConnection>

#include <kworkspace.h>

#include "ksystemactivitydialog.h"

int main(int argc, char **argv)
{
    KWorkSpace::detectPlatform(argc, argv);
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("systemmonitor");

    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setDesktopFileName(QStringLiteral("org.kde.systemmonitor"));

    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QDBusConnection con = QDBusConnection::sessionBus();
    if (!con.registerService(QStringLiteral("org.kde.systemmonitor"))) {
        return 0;
    }

    KSystemActivityDialog dialog;
    return dialog.exec();
}
