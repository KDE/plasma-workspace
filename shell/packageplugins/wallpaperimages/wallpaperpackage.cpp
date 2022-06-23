/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "wallpaperpackage.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <klocalizedstring.h>

WallpaperPackage::WallpaperPackage(QObject *parent, const QVariantList &args)
    : KPackage::PackageStructure(parent, args)
{
}

void WallpaperPackage::initPackage(KPackage::Package *package)
{
    package->addDirectoryDefinition("images", QStringLiteral("images/"), i18n("Images"));
    package->addDirectoryDefinition(QByteArrayLiteral("images_dark"),
                                    QStringLiteral("images_dark%1").arg(QDir::separator()),
                                    i18n("Images for dark color scheme"));

    QStringList mimetypes;
    mimetypes << QStringLiteral("image/avif") //
              << QStringLiteral("image/bmp") //
              << QStringLiteral("image/gif") //
              << QStringLiteral("image/heif") //
              << QStringLiteral("image/jpeg") //
              << QStringLiteral("image/jpg") //
              << QStringLiteral("image/png") //
              << QStringLiteral("image/svg") //
              << QStringLiteral("image/tiff") //
              << QStringLiteral("image/webp");
    package->setMimeTypes("images", mimetypes);
    package->setMimeTypes(QByteArrayLiteral("images_dark"), mimetypes);

    package->setRequired("images", true);
    package->setRequired(QByteArrayLiteral("images_dark"), false);
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
    package->removeDefinition(QByteArrayLiteral("preferredDark"));
    package->setRequired("images", isFullPackage);
    package->setRequired(QByteArrayLiteral("images_dark"), false);

    if (isFullPackage) {
        package->setContentsPrefixPaths(QStringList() << QStringLiteral("contents/"));
    } else {
        package->addFileDefinition("screenshot", info.fileName(), i18n("Preview"));
        package->addFileDefinition("preferred", info.fileName(), QString());
        package->addFileDefinition(QByteArrayLiteral("preferredDark"), info.fileName(), QString());
        package->setContentsPrefixPaths(QStringList());
        package->setPath(info.path());
    }

    guard = false;
}

K_PLUGIN_CLASS_WITH_JSON(WallpaperPackage, "plasma-packagestructure-wallpaperimages.json")

#include "wallpaperpackage.moc"
