/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QString>

namespace ImageBackendTestData
{
constexpr const int defaultImageCount = 2;
constexpr const int defaultPackageCount = 2;
constexpr int defaultVideoCount = 3;
constexpr int defaultTotalCount = defaultImageCount + defaultPackageCount + defaultVideoCount;

constexpr const int alternateImageCount = 1;
constexpr const int alternatePackageCount = 1;
constexpr int alternateVideoCount = 1;
constexpr int alternateTotalCount = alternateImageCount + alternatePackageCount + alternateVideoCount;

inline const QString defaultImageFileName1 = QStringLiteral("wallpaper.jpg.jpg");
inline const QString defaultImageFileName2 = QStringLiteral("# BUG454692 file name with hash char.png");
inline const QString defaultPackageFolderName1 = QStringLiteral("FEATURE207976-dark-wallpaper");
inline const QString defaultPackageFolderName2 = QStringLiteral("package");
inline const QString defaultVideoFileName1 = QStringLiteral("video.mp4");
inline const QString defaultVideoFileName2 = QStringLiteral("video.ogv");
inline const QString defaultVideoFileName3 = QStringLiteral("video.webm");

inline const QString alternateImageFileName1 = QStringLiteral("dummy.jpg");
inline const QString alternatePackageFolderName1 = QStringLiteral("dummy");
inline const QString alternateVideoFileName1 = QStringLiteral("dummy.ogv");
}
