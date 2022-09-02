/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

namespace BackgroundType
{
Q_NAMESPACE

enum class Type {
    Unknown,
    Image,
    AnimatedImage, /**< AnimatedImage doesn't support \QQuickImageProvider , @see https://bugreports.qt.io/browse/QTBUG-30524 */
};
Q_ENUM_NS(Type)
}
