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

    Q_INVOKABLE QString validate(const QString &input, const QString &other, int transitionDuration) const;
};
