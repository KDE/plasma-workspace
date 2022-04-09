/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QStringList>

QStringList suffixes();

/**
 * Check if the image format is supported by QImageReader.
 *
 * @return @p true if the format is supported, @p false otherwise.
 */
bool isAcceptableSuffix(const QString &suffix);
