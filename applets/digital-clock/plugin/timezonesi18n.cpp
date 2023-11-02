/*
    SPDX-FileCopyrightText: 2014 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "timezonesi18n.h"

#include <KCountry>
#include <KLocalizedString>

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

void TimezonesI18n::init()
{
    m_i18nContinents = TimezonesI18nData::timezoneContinentToL10nMap();

    m_isInitialized = true;
}
