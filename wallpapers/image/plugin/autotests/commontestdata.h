/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFile>
#include <QString>

namespace ImageBackendTestData
{
inline constexpr int defaultImageCount = 5;
inline constexpr int defaultPackageCount = 2;
inline constexpr int defaultTotalCount = defaultImageCount + defaultPackageCount;

inline constexpr int alternateImageCount = 1;
inline constexpr int alternatePackageCount = 1;
inline constexpr int alternateTotalCount = alternateImageCount + alternatePackageCount;

static const QString defaultHiddenImageFileName = QStringLiteral(".hiddenfolder/hidden.jpg");
static const QString defaultImageFileName1 = QStringLiteral("wallpaper.jpg.jpg");
static const QString defaultImageFileName2 = QStringLiteral("# BUG454692 file name with hash char.png");
static const QString defaultImageFileName3 = QStringLiteral(".BUG460287/BUG460287.webp");
static const QString defaultImageFileName4 = QStringLiteral(".BUG460287/BUG461940.webp");
static const QString defaultImageFileName5_orig = QStringLiteral("BUG454692 file name with backslash.png");
static const QString defaultImageFileName5 = QStringLiteral("\\ BUG454692 file name with backslash.png");
static const QString defaultPackageFolderName1 = QStringLiteral("FEATURE207976-dark-wallpaper");
static const QString defaultPackageFolderName2 = QStringLiteral("package");

static const QString alternateImageFileName1 = QStringLiteral("dummy.jpg");
static const QString alternatePackageFolderName1 = QStringLiteral("dummy");

static const QString customAccentColorPackage1 = QStringLiteral("testdata/customaccentcolor/case1/");
static const QString customAccentColorPackage2 = QStringLiteral("testdata/customaccentcolor/case2/");
}

// Fix illegal filename on Windows
#define renameBizarreFile(dataDir)                                                                                                                             \
    QFile bizarreFileOrig(dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5_orig));                                                         \
    QVERIFY(bizarreFileOrig.exists());                                                                                                                         \
    QVERIFY(bizarreFileOrig.rename(dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5)));

#define restoreBizarreFile(dataDir)                                                                                                                            \
    QFile bizarreFile(dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5));                                                                  \
    QVERIFY(bizarreFile.exists());                                                                                                                             \
    QVERIFY(bizarreFile.rename(dataDir.absoluteFilePath(ImageBackendTestData::defaultImageFileName5_orig)));
