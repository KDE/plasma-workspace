/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SORTINGMODE_H
#define SORTINGMODE_H

class SortingMode
{
    Q_GADGET

public:
    enum Mode {
        Random,
        Alphabetical,
        AlphabeticalReversed,
        Modified,
        ModifiedReversed,
    };
    Q_ENUM(Mode)
};

#endif
