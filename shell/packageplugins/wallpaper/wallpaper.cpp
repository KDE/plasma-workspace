/******************************************************************************
*   Copyright 2007-2009 by Aaron Seigo <aseigo@kde.org>                       *
*   Copyright 2013 by Sebastian Kügler <sebas@kde.org>                        *
*                                                                             *
*   This library is free software; you can redistribute it and/or             *
*   modify it under the terms of the GNU Library General Public               *
*   License as published by the Free Software Foundation; either              *
*   version 2 of the License, or (at your option) any later version.          *
*                                                                             *
*   This library is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU          *
*   Library General Public License for more details.                          *
*                                                                             *
*   You should have received a copy of the GNU Library General Public License *
*   along with this library; see the file COPYING.LIB.  If not, write to      *
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
*   Boston, MA 02110-1301, USA.                                               *
*******************************************************************************/

#include "wallpaper.h"


#include <KLocalizedString>
#include <KDeclarative/KDeclarative>

void QmlWallpaperPackage::initPackage(Plasma::Package *package)
{
    package->addFileDefinition("mainscript", "ui/main.qml", i18n("Main Script File"));
    package->setRequired("mainscript", true);

    QStringList platform = KDeclarative::KDeclarative::runtimePlatform();
    if (!platform.isEmpty()) {
        QMutableStringListIterator it(platform);
        while (it.hasNext()) {
            it.next();
            it.setValue("platformcontents/" + it.value());
        }

        platform.append("contents");
        package->setContentsPrefixPaths(platform);
    }

    package->setDefaultPackageRoot("plasma/wallpapers/");

    package->addDirectoryDefinition("images", "images", i18n("Images"));
    package->addDirectoryDefinition("theme", "theme", i18n("Themed Images"));
    QStringList mimetypes;
    mimetypes << "image/svg+xml" << "image/png" << "image/jpeg";
    package->setMimeTypes("images", mimetypes);
    package->setMimeTypes("theme", mimetypes);

    package->addDirectoryDefinition("config", "config", i18n("Configuration Definitions"));
    mimetypes.clear();
    mimetypes << "text/xml";
    package->setMimeTypes("config", mimetypes);

    package->addDirectoryDefinition("ui", "ui", i18n("User Interface"));

    package->addDirectoryDefinition("data", "data", i18n("Data Files"));

    package->addDirectoryDefinition("scripts", "code", i18n("Executable Scripts"));
    mimetypes.clear();
    mimetypes << "text/plain";
    package->setMimeTypes("scripts", mimetypes);

    package->addDirectoryDefinition("translations", "locale", i18n("Translations"));
}

K_EXPORT_PLASMA_PACKAGE_WITH_JSON(QmlWallpaperPackage, "plasma-packagestructure-wallpaper.json")

#include "wallpaper.moc"
