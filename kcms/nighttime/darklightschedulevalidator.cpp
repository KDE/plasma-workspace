/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "darklightschedulevalidator.h"

#include <QTime>

static const int secondsInDay = 86400;

DarkLightScheduleValidator::DarkLightScheduleValidator(QObject *parent)
    : QObject(parent)
{
}

QString DarkLightScheduleValidator::validate(const QString &input, const QString &other, int transitionDuration) const
{
    const QTime inputTime = QTime::fromString(input, QStringLiteral("hh:mm"));
    if (!inputTime.isValid()) {
        return input;
    }

    const QTime otherTime = QTime::fromString(other, QStringLiteral("hh:mm"));
    if (!otherTime.isValid()) {
        return input;
    }

    const int daylightDuration = std::abs(inputTime.secsTo(otherTime));
    const int maximumTransitionDuration = std::min(daylightDuration, secondsInDay - daylightDuration);
    if (transitionDuration < maximumTransitionDuration) {
        return input;
    }

    const QTime correctedTime = otherTime.addSecs((inputTime < otherTime ? -1 : 1) * transitionDuration);
    return correctedTime.toString(QStringLiteral("hh:mm"));
}
