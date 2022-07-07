/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

class Provider
{
    Q_GADGET

public:
    enum class Type {
        Unknown,
        Image,
        Package,
    };
    Q_ENUM(Type)
};
