/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

namespace SortingMode
{
Q_NAMESPACE

enum Mode {
    Random,
    Alphabetical,
    AlphabeticalReversed,
    Modified,
    ModifiedReversed,
};
Q_ENUM_NS(Mode)
}
