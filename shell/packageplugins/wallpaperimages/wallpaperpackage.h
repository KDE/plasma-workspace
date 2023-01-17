/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KPackage/PackageStructure>

/**
 * By specifying "X-KDE-PlasmaImageWallpaper-AccentColor" in metadata,
 * a wallpaper package can override the accent color when
 * "Accent color from wallpaper" is enabled.
 */
class WallpaperPackage : public KPackage::PackageStructure
{
    Q_OBJECT

public:
    explicit WallpaperPackage(QObject *parent = nullptr, const QVariantList &args = QVariantList());

    void initPackage(KPackage::Package *package) override;
    void pathChanged(KPackage::Package *package) override;
};
