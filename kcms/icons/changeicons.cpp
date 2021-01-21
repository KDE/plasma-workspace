/*
 *   Copyright 20016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "iconssettings.h"
#include <KLocalizedString>
#include <QApplication>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (argc != 2) {
        return 1;
    }

    // KNS will give us a path
    const QStringList args = app.arguments();
    QString themeName = args.last();
    int idx = themeName.lastIndexOf('/');
    if (idx >= 0) {
        themeName = themeName.mid(idx + 1);
    }

    IconsSettings settings;
    if (settings.theme() == themeName) {
        // In KNS this will be displayed as a warning in the UI
        qWarning().noquote() << i18n("Icon theme is already used");
    } else {
        settings.setTheme(themeName);
        settings.save();
    }
    return 0;
}
