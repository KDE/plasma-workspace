/*
    exampleutility.cpp
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <KLocalizedString>
#include <QCollator>
#include <QDateTime>
#include <QLocale>

class Utility
{
public:
    template<class T>
    inline static QString collateExample(const T &collate);
    inline static QString numericExample(const QLocale &locale);
    inline static QString timeExample(const QLocale &locale);
    inline static QString shortTimeExample(const QLocale &locale);
    inline static QString measurementExample(const QLocale &locale);
    inline static QString monetaryExample(const QLocale &locale);
};

template<class T>
QString Utility::collateExample(const T &collate)
{
    auto example{QStringLiteral("abcdefgxyzABCDEFGXYZÅåÄäÖöÅåÆæØø")};
    auto collator{QCollator{collate}};
    std::sort(example.begin(), example.end(), collator);
    return example;
}

QString Utility::monetaryExample(const QLocale &locale)
{
    return locale.toCurrencyString(24.00);
}

QString Utility::timeExample(const QLocale &locale)
{
    return i18n("%1 (long format)", locale.toString(QDateTime::currentDateTime())) + QLatin1Char('\n')
        + i18n("%1 (short format)", locale.toString(QDateTime::currentDateTime(), QLocale::ShortFormat));
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
