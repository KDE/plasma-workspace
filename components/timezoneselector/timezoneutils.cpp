// SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>
//
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QTimeZone>

#include "timezoneutils.h"

QList<QByteArray> TimeZoneUtils::availableTimeZoneIds() const
{
    return QTimeZone::availableTimeZoneIds();
}

#include "moc_timezoneutils.cpp"
