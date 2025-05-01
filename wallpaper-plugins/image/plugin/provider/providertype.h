/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

namespace Provider
{
Q_NAMESPACE

enum class Type {
    Unknown,
    Image,
    Package,
};
Q_ENUM_NS(Type)
}
