// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

#include "defaultwallpaper.h"

#include <KConfigGroup>
#include <KPackage/PackageLoader>
#include <Plasma/Theme>

KPackage::Package DefaultWallpaper::defaultWallpaperPackage()
{
    // Try from the look and feel package first, then from the plasma theme
    KPackage::Package lookAndFeelPackage = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    KConfigGroup cg(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("KDE"));
    const QString packageName = cg.readEntry("LookAndFeelPackage", QString());
    // If empty, it will be the default (currently Breeze)
    if (!packageName.isEmpty()) {
        lookAndFeelPackage.setPath(packageName);
    }

    KConfigGroup lnfDefaultsConfig = KConfigGroup(KSharedConfig::openConfig(lookAndFeelPackage.filePath("defaults")), QStringLiteral("Wallpaper"));

    const QString image = lnfDefaultsConfig.readEntry("Image", "");
    KPackage::Package package = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Wallpaper/Images"));

    if (!image.isEmpty()) {
        package.setPath(
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/%1").arg(image), QStandardPaths::LocateDirectory));
    }

    if (!package.isValid()) {
        // Try to get a default from the plasma theme
        Plasma::Theme theme;
        QString path = theme.wallpaperPath();
        int index = path.indexOf(QLatin1String("/contents/images/"));
        if (index > -1) { // We have file from package -> get path to package
            path = path.left(index);
        }
        package.setPath(path);
    }

    if (!package.isValid()) {
        // Use Next
        package.setPath(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("wallpapers/Next"), QStandardPaths::LocateDirectory));
    }

    return package;
}
