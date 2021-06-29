/*
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TRANSLATIONSSETTINGS_H
#define TRANSLATIONSSETTINGS_H

#include "translationssettingsbase.h"

class TranslationsSettings : public TranslationsSettingsBase
{
    Q_OBJECT
    Q_PROPERTY(QStringList configuredLanguages READ configuredLanguages WRITE setConfiguredLanguages NOTIFY configuredLanguagesChanged)
public:
    TranslationsSettings(QObject *parent = nullptr);
    ~TranslationsSettings() override;

    QStringList configuredLanguages() const;
    void setConfiguredLanguages(const QStringList &langs);

Q_SIGNALS:
    void configuredLanguagesChanged();
};

#endif // TRANSLATIONSSETTINGS_H
