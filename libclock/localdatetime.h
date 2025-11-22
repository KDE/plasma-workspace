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
    QML_STRUCTURED_VALUE

    /**
     * The date time to be formatted
     */
    Q_PROPERTY(QDateTime dateTime READ dateTime WRITE setDateTime)

    /**
     * The format string to use for formatting the date time
     */
    Q_PROPERTY(QString format READ format WRITE setFormat)

public:
    explicit LocalDateTime();
    Q_INVOKABLE explicit LocalDateTime(const QDateTime &dateTime, const QString &format);
    ~LocalDateTime();

    QDateTime dateTime() const;
    void setDateTime(const QDateTime &dateTime);

    QString format() const;
    void setFormat(const QString &format);

    /**
     * @return The localized datetime string
     */
    Q_INVOKABLE QString toString() const;

private:
    QDateTime m_dateTime;
    QString m_format;
};
