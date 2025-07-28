/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <qqmlintegration.h>

namespace SortingMode
{
Q_NAMESPACE
QML_ELEMENT

enum Mode {
    Random,
    Alphabetical,
    AlphabeticalReversed,
    Modified,
    ModifiedReversed,
};
Q_ENUM_NS(Mode)
}
