/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QObject>

class DarkLightScheduleValidator : public QObject
{
    Q_OBJECT

public:
    explicit DarkLightScheduleValidator(QObject *parent = nullptr);

    Q_INVOKABLE QTime validate(const QTime &input, const QTime &other, int transitionDuration) const;
};
