/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QVariantList>

class QDBusMessage;

namespace Decoder
{
QVariantList decode(const QDBusMessage &message);
}
