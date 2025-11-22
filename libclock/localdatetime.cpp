/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "localdatetime.h"

#include <QLocale>

LocalDateTime::LocalDateTime() = default;

LocalDateTime::LocalDateTime(const QDateTime &dateTime, const QString &format)
    : m_dateTime(dateTime)
    , m_format(format)
{
}

LocalDateTime::~LocalDateTime() = default;

QString LocalDateTime::toString() const
{
    return QLocale::system().toString(m_dateTime, m_format);
}

#include "moc_localdatetime.cpp"
