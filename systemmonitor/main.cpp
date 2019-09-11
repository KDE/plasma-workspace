/*
 * Copyright (C) 2014  Vishesh Handa <me@vhanda.in>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <QApplication>
#include <QDBusConnection>
#include <KLocalizedString>

#include <kworkspace.h>

#include "ksystemactivitydialog.h"

int main(int argc, char** argv)
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
