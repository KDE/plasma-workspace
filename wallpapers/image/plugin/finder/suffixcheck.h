/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QStringList>

QStringList suffixes();
QStringList &videoSuffixes();

/**
 * Check if the image format is supported by QImageReader.
 *
 * @return @p true if the format is supported, @p false otherwise.
 */
bool isAcceptableSuffix(const QString &suffix);

/**
 * Checks if the video format is supported.
 *
 * @return @p true if the format is supported, @p false otherwise.
 */
bool isAcceptableVideoSuffix(const QString &suffix);
