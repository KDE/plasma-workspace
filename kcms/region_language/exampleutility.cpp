/*
    exampleutility.cpp
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "exampleutility.h"

QString Utility::monetaryExample(const QLocale &locale)
{
    return locale.toCurrencyString(24.00);
}

QString Utility::timeExample(const QLocale &locale)
{
    return locale.toString(QDateTime::currentDateTime()) + QLatin1Char('\n') + locale.toString(QDateTime::currentDateTime(), QLocale::ShortFormat);
}

QString Utility::shortTimeExample(const QLocale &locale)
{
    return locale.toString(QDateTime::currentDateTime(), QLocale::LongFormat);
}

QString Utility::measurementExample(const QLocale &locale)
{
    QString measurementExample;
    if (locale.measurementSystem() == QLocale::ImperialUKSystem) {
        measurementExample = i18nc("Measurement combobox", "Imperial UK");
    } else if (locale.measurementSystem() == QLocale::ImperialUSSystem || locale.measurementSystem() == QLocale::ImperialSystem) {
        measurementExample = i18nc("Measurement combobox", "Imperial US");
    } else {
        measurementExample = i18nc("Measurement combobox", "Metric");
    }
    return measurementExample;
}

QString Utility::numericExample(const QLocale &locale)
{
    return locale.toString(1000.01);
}

QString Utility::paperSizeExample(const QLocale &locale)
{
    QString paperSizeExample;
    if (locale.measurementSystem() == QLocale::ImperialUSSystem || locale.measurementSystem() == QLocale::ImperialSystem) {
        paperSizeExample = i18nc("PaperSize combobox", "Letter");
    } else {
        paperSizeExample = i18nc("PaperSize combobox", "A4");
    }
    return paperSizeExample;
}
