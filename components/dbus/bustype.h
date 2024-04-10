/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QObject>
#include <qqmlregistration.h>

namespace BusType
{
QML_ELEMENT
Q_NAMESPACE //
    enum Type {
        Session = 0,
        System,
    };
Q_ENUM_NS(Type)
}
