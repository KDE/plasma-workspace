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

#include <KDeclarative/KDeclarative>
#include <KLocalizedString>

void QmlWallpaperPackage::initPackage(KPackage::Package *package)
{
    package->addFileDefinition("mainscript", QStringLiteral("ui/main.qml"), i18n("Main Script File"));
    package->setRequired("mainscript", true);

    QStringList platform = KDeclarative::KDeclarative::runtimePlatform();
    if (!platform.isEmpty()) {
        QMutableStringListIterator it(platform);
        while (it.hasNext()) {
            it.next();
            it.setValue("platformcontents/" + it.value());
        }

        platform.append(QStringLiteral("contents"));
        package->setContentsPrefixPaths(platform);
    }

    package->setDefaultPackageRoot(QStringLiteral("plasma/wallpapers/"));

    package->addDirectoryDefinition("images", QStringLiteral("images"), i18n("Images"));
    package->addDirectoryDefinition("theme", QStringLiteral("theme"), i18n("Themed Images"));
    QStringList mimetypes;
    mimetypes << QStringLiteral("image/svg+xml") << QStringLiteral("image/png") << QStringLiteral("image/jpeg");
    package->setMimeTypes("images", mimetypes);
    package->setMimeTypes("theme", mimetypes);

    package->addDirectoryDefinition("config", QStringLiteral("config"), i18n("Configuration Definitions"));
    mimetypes.clear();
    mimetypes << QStringLiteral("text/xml");
    package->setMimeTypes("config", mimetypes);

    package->addDirectoryDefinition("ui", QStringLiteral("ui"), i18n("User Interface"));

    package->addDirectoryDefinition("data", QStringLiteral("data"), i18n("Data Files"));

    package->addDirectoryDefinition("scripts", QStringLiteral("code"), i18n("Executable Scripts"));
    mimetypes.clear();
    mimetypes << QStringLiteral("text/plain");
    package->setMimeTypes("scripts", mimetypes);

    package->addDirectoryDefinition("translations", QStringLiteral("locale"), i18n("Translations"));
}

K_EXPORT_KPACKAGE_PACKAGE_WITH_JSON(QmlWallpaperPackage, "plasma-packagestructure-wallpaper.json")

#include "wallpaper.moc"
