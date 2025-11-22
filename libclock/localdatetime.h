/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDateTime>
#include <QtQml/qqmlregistration.h>

/**
 * @brief A QML value type for handling localized date time formatting
 *
 * This class provides a value type to format QDateTime values according to the current locale
 */
class LocalDateTime
{
    Q_GADGET
    QML_VALUE_TYPE(localDateTime)
    QML_CONSTRUCTIBLE_VALUE

public:
    explicit LocalDateTime();
    Q_INVOKABLE explicit LocalDateTime(const QDateTime &dateTime, const QString &format);
    ~LocalDateTime();

    /**
     * @return The localized datetime string
     */
    Q_INVOKABLE QString toString() const;

private:
    /**
     * The date time to be formatted
     */
    QDateTime m_dateTime;

    /**
     * The format string to use for formatting the date time
     */
    QString m_format;
};
