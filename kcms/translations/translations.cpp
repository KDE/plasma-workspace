/*
 *  Copyright (C) 2014 John Layt <john@layt.net>
 *  Copyright (C) 2018 Eike Hein <hein@kde.org>
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

#include "translations.h"
#include "translationsdata.h"
#include "translationsmodel.h"
#include "translationssettings.h"

#include <KAboutData>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

K_PLUGIN_FACTORY_WITH_JSON(TranslationsFactory, "kcm_translations.json", registerPlugin<Translations>(); registerPlugin<TranslationsData>();)

Translations::Translations(QObject *parent, const QVariantList &args)
    : KQuickAddons::ManagedConfigModule(parent, args)
    , m_data(new TranslationsData(this))
    , m_translationsModel(new TranslationsModel(this))
    , m_everSaved(false)
{
    auto *about = new KAboutData(QStringLiteral("kcm_translations"), i18n("Language"), QStringLiteral("2.0"), QString(), KAboutLicense::LGPL);
    setAboutData(about);

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
    emit everSavedChanged();
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
