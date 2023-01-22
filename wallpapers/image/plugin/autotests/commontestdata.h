/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QString>

namespace ImageBackendTestData
{
constexpr int defaultImageCount = 5;
constexpr int defaultPackageCount = 2;
constexpr int defaultTotalCount = defaultImageCount + defaultPackageCount;

constexpr int alternateImageCount = 1;
constexpr int alternatePackageCount = 1;
constexpr int alternateTotalCount = alternateImageCount + alternatePackageCount;

inline const QString defaultHiddenImageFileName = QStringLiteral(".hiddenfolder/hidden.jpg");
inline const QString defaultImageFileName1 = QStringLiteral("wallpaper.jpg.jpg");
inline const QString defaultImageFileName2 = QStringLiteral("# BUG454692 file name with hash char.png");
inline const QString defaultImageFileName3 = QStringLiteral(".BUG460287/BUG460287.webp");
inline const QString defaultImageFileName4 = QStringLiteral(".BUG460287/BUG461940.webp");
inline const QString defaultImageFileName5 = QStringLiteral("\\ BUG454692 file name with backslash.png");
inline const QString defaultPackageFolderName1 = QStringLiteral("FEATURE207976-dark-wallpaper");
inline const QString defaultPackageFolderName2 = QStringLiteral("package");

inline const QString alternateImageFileName1 = QStringLiteral("dummy.jpg");
inline const QString alternatePackageFolderName1 = QStringLiteral("dummy");

inline const QString customAccentColorPackage1 = QStringLiteral("testdata/customaccentcolor/case1/");
inline const QString customAccentColorPackage2 = QStringLiteral("testdata/customaccentcolor/case2/");
}
