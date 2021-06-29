/*
    SPDX-FileCopyrightText: 2014 John Layt <john@layt.net>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KQuickAddons/ManagedConfigModule>

class AvailableTranslationsModel;
class SelectedTranslationsModel;
class TranslationsModel;
class TranslationsSettings;
class TranslationsData;

class Translations : public KQuickAddons::ManagedConfigModule
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *translationsModel READ translationsModel CONSTANT)
    Q_PROPERTY(bool everSaved READ everSaved NOTIFY everSavedChanged)

public:
    explicit Translations(QObject *parent = nullptr, const QVariantList &list = QVariantList());
    ~Translations() override;

    QAbstractItemModel *translationsModel() const;

    bool everSaved() const;
    TranslationsSettings *settings() const;

public Q_SLOTS:
    void load() override;
    void save() override;
    void defaults() override;

Q_SIGNALS:
    void everSavedChanged() const;

private Q_SLOTS:
    void selectedLanguagesChanged();

private:
    bool isSaveNeeded() const override;

    TranslationsData *m_data;
    TranslationsModel *m_translationsModel;

    bool m_everSaved;
};
