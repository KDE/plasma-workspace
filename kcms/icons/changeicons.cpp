/*
    SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "iconssettings.h"
#include "plasma_changeicons_debug.h"

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
        qCWarning(PLASMA_CHANGEICONS_DEBUG).noquote() << "Icon theme is already used";
    } else {
        settings.setTheme(themeName);
        settings.save();
    }
    return 0;
}
