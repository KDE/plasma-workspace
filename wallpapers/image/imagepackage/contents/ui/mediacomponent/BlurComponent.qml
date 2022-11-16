/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtGraphicalEffects 1.15

/**
 * QtGraphicalEffects is gone in Qt6, so put it in a Loader to avoid blank wallpapers.
 * TODO Qt 6
 */
FastBlur {
    source: backgroundColor.blurSource
    radius: 32
}
