/*
 *  Copyright (C) 2019 Kevin Ottens <kevin.ottens@enioka.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
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
    return languageString().split(QLatin1Char(':'), QString::SkipEmptyParts);
}

void TranslationsSettings::setConfiguredLanguages(const QStringList &langs)
{
    setLanguageString(langs.join(QLatin1Char(':')));
}
