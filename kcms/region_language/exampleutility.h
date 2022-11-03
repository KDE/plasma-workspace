/*
    exampleutility.h
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KLocalizedString>
#include <KMacroExpander>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QTextStream>
#include <langinfo.h>

class Utility
{
    friend class ExampleUtilityTest;
#ifdef LC_ADDRESS
    static QString resolveFieldDescriptors(QHash<QChar, QString> map, int langInfoFormat, int lcFormat, const QLocale &locale);
    static QString getLocaleInfo(int langInfoFormat, int lcFormat, const QLocale &locale);
    static QString parseLocaleFile(QString localeName, int langInfoFormat);
    static QFileInfo findLocaleInFolder(QString localeName, QString localeDirectory);
    static QString getFormatToFetch(int langInfoFormat);
    static QStringList getLangCodeFromLocale(QLocale locale);
    static QString replaceASCIIUnicodeSymbol(const QString &string);
#endif

public:
    static QString numericExample(const QLocale &locale);
    static QString timeExample(const QLocale &locale);
    static QString shortTimeExample(const QLocale &locale);
    static QString measurementExample(const QLocale &locale);
    static QString monetaryExample(const QLocale &locale);
    static QString paperSizeExample(const QLocale &locale);
#ifdef LC_ADDRESS
    static QString addressExample(const QLocale &locale);
    static QString nameStyleExample(const QLocale &locale);
    static QString phoneNumbersExample(const QLocale &locale);
#endif
};
