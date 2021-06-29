/*
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "translationssettings.h"

TranslationsSettings::TranslationsSettings(QObject *parent)
    : TranslationsSettingsBase(parent)
{
    connect(this, &TranslationsSettingsBase::languageStringChanged, this, &TranslationsSettings::configuredLanguagesChanged);
}

TranslationsSettings::~TranslationsSettings()
{
}

QStringList TranslationsSettings::configuredLanguages() const
{
    return languageString().split(QLatin1Char(':'), Qt::SkipEmptyParts);
}

void TranslationsSettings::setConfiguredLanguages(const QStringList &langs)
{
    setLanguageString(langs.join(QLatin1Char(':')));
}
