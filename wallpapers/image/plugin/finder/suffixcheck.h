/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QStringList>

const QStringList &suffixes();

/**
 * Check if a path is inside a wallpaper package's image directory.
 *
 * Such images belong to the package, not to the list of standalone wallpapers.
 */
bool isPackageImagePath(const QString &path);

/**
 * Check if the image format is supported by QImageReader.
 *
 * @return @p true if the format is supported, @p false otherwise.
 */
bool isAcceptableSuffix(QString &&suffix);
