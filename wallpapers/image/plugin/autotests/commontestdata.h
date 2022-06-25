/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QString>

namespace ImageBackendTestData
{
constexpr const int defaultImageCount = 2;
constexpr const int defaultPackageCount = 3;
constexpr const int defaultTotalCount = defaultImageCount + defaultPackageCount;

constexpr const int alternateImageCount = 1;
constexpr const int alternatePackageCount = 1;
constexpr const int alternateTotalCount = alternateImageCount + alternatePackageCount;

inline const QString defaultImageFileName1 = QStringLiteral("wallpaper.jpg.jpg");
inline const QString defaultImageFileName2 = QStringLiteral("# BUG454692 file name with hash char.png");
inline const QString defaultPackageFolderName1 = QStringLiteral("FEATURE199001-Timed");
inline const QString defaultPackageFolderName2 = QStringLiteral("FEATURE207976-dark-wallpaper");
inline const QString defaultPackageFolderName3 = QStringLiteral("package");

inline const QString alternateImageFileName1 = QStringLiteral("dummy.jpg");
inline const QString alternatePackageFolderName1 = QStringLiteral("dummy");
}
