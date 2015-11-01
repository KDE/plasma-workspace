/*
 *   Copyright 2013 by Marco Martin <mart@kde.org>

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

#include "wallpaperpackage.h"

#include <QFileInfo>
#include <QDebug>

#include <klocalizedstring.h>

WallpaperPackage::WallpaperPackage(QObject *parent, const QVariantList &args)
    : KPackage::PackageStructure(parent, args)
{
}

void WallpaperPackage::initPackage(KPackage::Package *package)
{
    package->addDirectoryDefinition("images", QStringLiteral("images/"), i18n("Images"));

    QStringList mimetypes;
    mimetypes << QStringLiteral("image/svg") << QStringLiteral("image/png") << QStringLiteral("image/jpeg") << QStringLiteral("image/jpg");
    package->setMimeTypes("images", mimetypes);

    package->setRequired("images", true);
    package->addFileDefinition("screenshot", QStringLiteral("screenshot.png"), i18n("Screenshot"));
    package->setAllowExternalPaths(true);
}

void WallpaperPackage::pathChanged(KPackage::Package *package)
{
    static bool guard = false;

    if (guard) {
        return;
    }

    guard = true;
    QString ppath = package->path();
    if (ppath.endsWith('/')) {
        ppath.chop(1);
        if (!QFile::exists(ppath)) {
            ppath = package->path();
        }
    }

    QFileInfo info(ppath);
    const bool isFullPackage = info.isDir();
    package->removeDefinition("preferred");
    package->setRequired("images", isFullPackage);

    if (isFullPackage) {
        package->setContentsPrefixPaths(QStringList() << QStringLiteral("contents/"));
    } else {
        package->addFileDefinition("screenshot", info.fileName(), i18n("Preview"));
        package->addFileDefinition("preferred", info.fileName(), QString());
        package->setContentsPrefixPaths(QStringList());
        package->setPath(info.path());
    }

    guard = false;
}

K_EXPORT_KPACKAGE_PACKAGE_WITH_JSON(WallpaperPackage, "plasma-packagestructure-wallpaperimages.json")

#include "wallpaperpackage.moc"

