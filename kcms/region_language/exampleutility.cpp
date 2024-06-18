/*
    exampleutility.cpp
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "exampleutility.h"

using namespace Qt::StringLiterals;

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
    // clang-format off
    // these locales have Metric measurement system but Letter paper size
    const QSet<QString> letterLocales {
        QStringLiteral("en_CA"),
        QStringLiteral("en_PH"),
        QStringLiteral("es_BO"),
        QStringLiteral("es_CL"),
        QStringLiteral("es_CO"),
        QStringLiteral("es_CR"),
        QStringLiteral("es_GT"),
        QStringLiteral("es_MX"),
        QStringLiteral("es_NI"),
        QStringLiteral("es_PA"),
        QStringLiteral("es_PR"),
        QStringLiteral("es_SV"),
        QStringLiteral("es_VE"),
        QStringLiteral("fil_PH"),
        QStringLiteral("fr_CA"),
        QStringLiteral("ik_CA"),
        QStringLiteral("iu_CA"),
        QStringLiteral("miq_NI"),
        QStringLiteral("nhn_MX"),
        QStringLiteral("shs_CA"),
        QStringLiteral("tl_PH")
    };
    // clang-format on
    QString paperSizeExample;
    if (letterLocales.contains(locale.name()) || locale.measurementSystem() == QLocale::ImperialUSSystem
        || locale.measurementSystem() == QLocale::ImperialSystem) {
        paperSizeExample = i18nc("PaperSize combobox", "Letter");
    } else {
        paperSizeExample = i18nc("PaperSize combobox", "A4");
    }
    return paperSizeExample;
}

// If LC_ADDRESS does not exist, we do not use these at all
// The format characters are found from here https://lh.2xlibre.net/
#ifdef LC_ADDRESS
QString Utility::addressExample(const QLocale &locale)
{
    // Create an example string using POSTAL_FMT
    const QStringList lang = getLangCodeFromLocale(locale);
    const QHash<QChar, QString> map{
        // QChar is the field descriptor and QString is the value
        {u'n', nameStyleExample(locale)}, // Person's Name, use LC_NAME for this
        {u'a', ki18nc("Care of person or organization", "c/o").toString(lang)},
        {u'f', ki18nc("Firm name", "Acme Corporation").toString(lang)},
        {u'd', ki18nc("Department name", "Development Department").toString(lang)},
        {u'b', ki18nc("Building name", "Dev-Building").toString(lang)},
        {u's', ki18nc("Street or block name", "Main Street").toString(lang)},
        {u'h', ki18nc("House number", "House 1").toString(lang)},
        {u'N', u"\n"_s}, // End of line
        {u't', ki18nc("Whitespace field for locale address style example", " ").toString(lang)}, // Space
        {u'r', ki18nc("Room number", "Room 2").toString(lang)},
        {u'e', ki18nc("Floor number", "Floor 3").toString(lang)},
        {u'C', getLocaleInfo(_NL_ADDRESS_COUNTRY_POST, LC_ADDRESS, locale)}, // Country designation from the CountryKeyword
        {u'l', ki18nc("Local township within town or city", "Downtown").toString(lang)},
        {u'z', ki18nc("Zip number, postal code", "123456").toString(lang)},
        {u'T', ki18nc("Town or city", "City").toString(lang)},
        {u'S', ki18nc("State, province or prefecture", "State").toString(lang)},
        {u'c', getLocaleInfo(_NL_ADDRESS_COUNTRY_NAME, LC_ADDRESS, locale)}, // Country from data record
    };

    return resolveFieldDescriptors(map, _NL_ADDRESS_POSTAL_FMT, LC_ADDRESS, locale);
}
#endif

#ifdef LC_ADDRESS
QString Utility::nameStyleExample(const QLocale &locale)
{
    const QStringList lang = getLangCodeFromLocale(locale);
    const QHash<QChar, QString> map{
        {u'f', ki18nc("Family names", "FamilyName").toString(lang)},
        {u'F', ki18nc("Family names in uppercase", "FAMILYNAME").toString(lang)},
        {u'g', ki18nc("First given name", "FirstName").toString(lang)},
        {u'G', ki18nc("First given initial", "F").toString(lang)},
        {u'l', ki18nc("First given name with latin letters", "FirstName").toString(lang)},
        {u'o', ki18nc("Other shorter name", "OtherName").toString(lang)},
        {u'm', ki18nc("Additional given names", "AdditionalName").toString(lang)},
        {u'M', ki18nc("Initials for additional given names", "A").toString(lang)},
        {u'p', ki18nc("Profession", "Profession").toString(lang)},
        {u's', ki18nc("Salutation", "Doctor").toString(lang)},
        {u'S', ki18nc("Abbreviated salutation", "Dr.").toString(lang)},
        {u'd', ki18nc("Salutation using the FDCC-sets conventions", "Dr.").toString(lang)},
        {u't', ki18nc("Space or dot for locale name style example", " ").toString(lang)}, // Space or dot. Space produces better examples.
    };
    return resolveFieldDescriptors(map, _NL_NAME_NAME_FMT, LC_NAME, locale);
}
#endif

#ifdef LC_ADDRESS
QString Utility::phoneNumbersExample(const QLocale &locale)
{
    const QHash<QChar, QString> map = {
        {u'a', u"123"_s}, // Area code without nationwide prefix
        {u'A', u"0123"_s}, // Area code with nationwide prefix
        {u'l', u"1234567"_s}, // Local number within area code
        {u'e', u"321"_s}, // Extension to local number
        {u'c', getLocaleInfo(_NL_TELEPHONE_INT_PREFIX, LC_TELEPHONE, locale)}, // Country code
        {u'C', u"01"_s}, // Alternate carrier service code used for dialling abroad
        {u't', ki18nc("Whitespace for telephone style example", " ").toString(getLangCodeFromLocale(locale))}, // Insert space
    };

    return resolveFieldDescriptors(map, _NL_TELEPHONE_TEL_INT_FMT, LC_TELEPHONE, locale);
}
#endif

#ifdef LC_ADDRESS
QString Utility::resolveFieldDescriptors(const QHash<QChar, QString> &map, int langInfoFormat, int lcFormat, const QLocale &locale)
{
    QString formatString = getLocaleInfo(langInfoFormat, lcFormat, locale);
    QString example = KMacroExpander::expandMacros(formatString, map);

    if (example.isEmpty() || example == QLatin1String("???")) {
        return i18nc("This is returned when an example test could not be made from locale information", "Could not find an example for this locale");
    }
    return example;
}

QString Utility::getLocaleInfo(int langInfoFormat, int lcFormat, const QLocale &locale)
{
    QString localeInfo = parseLocaleFile(locale.name(), langInfoFormat);

    if (localeInfo.isEmpty()) {
        const QString localeString = locale.name() + QLatin1String(".UTF-8");
        const QByteArray localeByteArray = localeString.toUtf8();

        if (setlocale(lcFormat, localeByteArray.constData())) {
            localeInfo = QString::fromUtf8(nl_langinfo(langInfoFormat));
        }
    }

    return localeInfo;
}

QString Utility::parseLocaleFile(const QString &localeName, int langInfoFormat)
{
    static std::unordered_map<QString, QString> resultCache;

    const QString formatToFetch = getFormatToFetch(langInfoFormat);
    const QString cacheKey = formatToFetch + u"###" + localeName;
    if (resultCache.contains(cacheKey)) {
        return resultCache[cacheKey];
    }

    // insert an empty value here, so we ensure one file only parsed once
    resultCache.insert({cacheKey, QString()});

    QFileInfo localeFileInfo;

    // Get the locale file info from the first folder where it's found
    for (const auto &localeDirectory : {QStringLiteral("/usr/share/i18n/locales/")}) {
        localeFileInfo = findLocaleInFolder(localeName, localeDirectory);
        if (localeFileInfo.exists()) {
            break;
        }
    }

    // Parse through file and return the inquired field descriptor
    if (localeFileInfo.exists()) {
        QFile localeFile(localeFileInfo.filePath());
        // Return if we cant open file
        if (!localeFile.open(QIODevice::ReadOnly)) {
            return {};
        }

        QTextStream textStream(&localeFile);

        if (formatToFetch.isEmpty()) {
            return {};
        }
        // Read the file with regex and return the first match

        const QRegularExpression rx(QString(formatToFetch + u"\\s+\"(.*)\""));

        while (!textStream.atEnd()) {
            QString line = textStream.readLine();
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                // Return the first (and only) match
                const QString result = replaceASCIIUnicodeSymbol(match.captured(1));
                resultCache[cacheKey] = result;
                return result;
            }
        }
    }

    return {};
}

// Glibc store unicode char as ASCII symbol, 'T<U00FC>rkiye' for 'Türkiye'
QString Utility::replaceASCIIUnicodeSymbol(const QString &string)
{
    int i = 0;
    int literalStringStart = 0;
    int unicodeStart = 0;
    QString result;
    result.reserve(string.size());
    bool replacing = false;

    /*
     * T<U00FC>rkiye
     * ||     ||-State 1
     * ||     |- State 3, we check if the code block is valid, then added the converted char
     * ||        to the result, reset literal string start to the position after itself
     * ||- State 2, we add the literal string before it to the result, set 'unicodeStart' to the position after 'U'
     * |- State 1, literal string state
     * */
    while (i < string.size()) {
        if (replacing && i > unicodeStart && string[i] == QLatin1Char('>')) {
            bool ok = false;
            QStringView section(string);
            // convert base 16 string to utf-16 value
            auto unicodePoint = section.mid(unicodeStart, i - unicodeStart).toInt(&ok, 16);
            if (ok && QChar::isPrint(unicodePoint)) {
                result.append(QChar(unicodePoint));
                literalStringStart = i + 1;
            }
            replacing = false;
        } else if (string[i] == QLatin1Char('<') && i + 1 < string.size() && string[i + 1] == QLatin1Char('U')) {
            // append literal string before unicode block
            result.append(QStringView(string).mid(literalStringStart, i - literalStringStart));
            replacing = true;
            // <U00FF>, we ignore 'U'
            unicodeStart = i + 2;
        }
        i++;
    }
    result.append(QStringView(string).mid(literalStringStart, i - literalStringStart));
    return result;
}

QFileInfo Utility::findLocaleInFolder(const QString &localeName, const QString &localeDirectory)
{
    QDirIterator dirIterator(localeDirectory);
    // Iterate through files in the locale directory
    while (dirIterator.hasNext()) {
        QString fileName = dirIterator.next();
        QFileInfo fileInfo(fileName);

        // Ignore directories
        if (fileInfo.isDir()) {
            continue;
        }

        // If file is found, break the loop
        if (fileInfo.fileName().startsWith(localeName)) {
            return fileInfo;
            break;
        }
    }
    return {};
}

QString Utility::getFormatToFetch(int langInfoFormat)
{
    switch (langInfoFormat) {
    case _NL_ADDRESS_POSTAL_FMT:
        return QStringLiteral("postal_fmt");
    case _NL_ADDRESS_COUNTRY_POST:
        return QStringLiteral("country_post");
    case _NL_ADDRESS_COUNTRY_NAME:
        return QStringLiteral("country_name");
    case _NL_NAME_NAME_FMT:
        return QStringLiteral("name_fmt");
    case _NL_TELEPHONE_TEL_INT_FMT:
        return QStringLiteral("tel_int_fmt");
    case _NL_TELEPHONE_INT_PREFIX:
        return QStringLiteral("int_prefix");
    }
    return {};
}

QStringList Utility::getLangCodeFromLocale(const QLocale &locale)
{
    QStringList languages;
    for (QString language : locale.uiLanguages()) {
        language.replace(QLatin1Char('-'), QLatin1Char('_'));
        languages += language;
    }
    // `uiLanguages` don't offer the minimal version for some locale, such as fr_DZ.
    if (auto pos = languages.last().indexOf(QLatin1Char('_')); pos >= 0) {
        languages += languages.last().left(pos);
    }
    return languages;
}
#endif
