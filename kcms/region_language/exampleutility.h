/*
    exampleutility.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KLocalizedString>
#include <QDateTime>
#include <QLocale>

class Utility
{
public:
    static QString numericExample(const QLocale &locale);
    static QString timeExample(const QLocale &locale);
    static QString shortTimeExample(const QLocale &locale);
    static QString measurementExample(const QLocale &locale);
    static QString monetaryExample(const QLocale &locale);
};
