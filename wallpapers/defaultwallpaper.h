// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

#pragma once

#include <KPackage/Package>

namespace DefaultWallpaper
{
/**
 * @returns the package containing the default wallpaper
 */
[[nodiscard]] KPackage::Package defaultWallpaperPackage();
} // namespace DefaultWallpaper
