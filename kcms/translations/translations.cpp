/*
    SPDX-FileCopyrightText: 2014 John Layt <john@layt.net>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "translations.h"
#include "translationsdata.h"
#include "translationsmodel.h"
#include "translationssettings.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

K_PLUGIN_CLASSES_WITH_JSON(Translations, TranslationsData, "kcm_translations.json")

Translations::Translations(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, data, args)
    , m_data(new TranslationsData(this))
    , m_translationsModel(new TranslationsModel(this))
    , m_everSaved(false)
{
    setButtons(Apply | Default);

    connect(m_translationsModel, &TranslationsModel::selectedLanguagesChanged, this, &Translations::selectedLanguagesChanged);
}

Translations::~Translations()
{
}

QAbstractItemModel *Translations::translationsModel() const
{
    return m_translationsModel;
}

bool Translations::everSaved() const
{
    return m_everSaved;
}

void Translations::load()
{
    KQuickAddons::ManagedConfigModule::load();
    m_translationsModel->setSelectedLanguages(settings()->configuredLanguages());
}

void Translations::save()
{
    m_everSaved = true;
    Q_EMIT everSavedChanged();
    KQuickAddons::ManagedConfigModule::save();
}

void Translations::defaults()
{
    KQuickAddons::ManagedConfigModule::defaults();
    m_translationsModel->setSelectedLanguages(settings()->configuredLanguages());
}

void Translations::selectedLanguagesChanged()
{
    auto configuredLanguages = m_translationsModel->selectedLanguages();

    const auto missingLanguages = m_translationsModel->missingLanguages();
    for (const auto &lang : missingLanguages) {
        configuredLanguages.removeOne(lang);
    }

    settings()->setConfiguredLanguages(configuredLanguages);
}

TranslationsSettings *Translations::settings() const
{
    return m_data->settings();
}

bool Translations::isSaveNeeded() const
{
    return !m_translationsModel->missingLanguages().isEmpty();
}

#include "translations.moc"
