/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <qqmlintegration.h>

namespace BackgroundType
{
Q_NAMESPACE
QML_ELEMENT

enum class Type {
    Unknown,
    Image,
    AnimatedImage,
    VectorImage, // VectorImage is the only one that uses \QQuickImageProvider, because if QQuickImageProvider is used, then Image considers it always scalable
    DayNight,
};
Q_ENUM_NS(Type)
}
