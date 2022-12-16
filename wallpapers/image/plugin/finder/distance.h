/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

class QSize;
class QString;

/**
 * Compute difference of areas
 */
float distance(const QSize &size, const QSize &desired);

/**
 * @return size from the filename
 */
QSize resSize(const QString &str);
