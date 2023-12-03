/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "timezonesi18n.h"

#include <KCountry>
#include <KLocalizedString>

#include <unicode/localebuilder.h>

#include "timezonesi18n_generated.h"

TimezonesI18n::TimezonesI18n(QObject *parent)
    : QObject(parent)
    , m_isInitialized(false)
{
}

QString TimezonesI18n::i18nContinents(const QString &continent)
{
    if (!m_isInitialized) {
        init();
    }
    return m_i18nContinents.value(continent, continent);
}

QString TimezonesI18n::i18nCountry(QLocale::Country country)
{
    if (!m_isInitialized) {
        init();
    }

    QString countryName = KCountry::fromQLocale(country).name();

    if (countryName.isEmpty()) {
        return QLocale::countryToString(country);
    }

    return countryName;
}

QString TimezonesI18n::i18nCity(const QString &timezoneId)
{
    if (!m_isInitialized) {
        init();
    }

    if (!m_tzNames) {
        return timezoneId;
    }

    icu::UnicodeString result;
    const auto &cityName = m_tzNames->getExemplarLocationName(icu::UnicodeString::fromUTF8(icu::StringPiece(timezoneId.toStdString())), result);

    return cityName.isBogus() ? timezoneId : QString::fromUtf16(cityName.getBuffer(), cityName.length());
}

void TimezonesI18n::init()
{
    m_i18nContinents = TimezonesI18nData::timezoneContinentToL10nMap();

    const auto locale = icu::Locale(QLocale::system().name().toLatin1());
    UErrorCode error = U_ZERO_ERROR;
    m_tzNames.reset(icu::TimeZoneNames::createInstance(locale, error));
    if (!U_SUCCESS(error)) {
        qWarning() << "failed to create timezone names:" << u_errorName(error);
    }

    m_isInitialized = true;
}
