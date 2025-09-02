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

QTime DarkLightScheduleValidator::validate(const QTime &inputTime, const QTime &otherTime, int transitionDuration) const
{
    if (!inputTime.isValid()) {
        return inputTime;
    }

    if (!otherTime.isValid()) {
        return inputTime;
    }

    const int daylightDuration = std::abs(inputTime.secsTo(otherTime));
    const int maximumTransitionDuration = std::min(daylightDuration, secondsInDay - daylightDuration);
    if (transitionDuration < maximumTransitionDuration) {
        return inputTime;
    }

    return otherTime.addSecs((inputTime < otherTime ? -1 : 1) * transitionDuration);
}
