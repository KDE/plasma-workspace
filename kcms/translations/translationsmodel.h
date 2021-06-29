/*
    SPDX-FileCopyrightText: 2014 John Layt <john@layt.net>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>

#include <KLocalizedString>

#include "language.h"

using LanguageVector = QVector<Language *>;

class TranslationsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList selectedLanguages READ selectedLanguages WRITE setSelectedLanguages NOTIFY selectedLanguagesChanged)
    Q_PROPERTY(QStringList missingLanguages READ missingLanguages NOTIFY missingLanguagesChanged)
public:
    enum AdditionalRoles {
        Object = Qt::UserRole + 1,
        LanguageCode,
        IsSelected, // mutable whether the language is marked for use
        SelectionPreference, // integer index of selection preference (establishes the order of selections)
        IsIncomplete, // whether the language is missing distro packages (not relevant when not IsSelected)
        IsInstalling, // only true when the language was incomplete and is on the way to completion
    };
    Q_ENUM(AdditionalRoles)

    using QAbstractListModel::QAbstractListModel;

    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QStringList selectedLanguages() const;
    void setSelectedLanguages(const QStringList &languages);

    QStringList missingLanguages() const;

    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void remove(const QString &languageCode);

Q_SIGNALS:
    void selectedLanguagesChanged(const QStringList &languages) const;
    void missingLanguagesChanged() const;

private:
    QString languageCodeToName(const QString &languageCode) const;
    LanguageVector makeLanguages(const QStringList &codes);

    // The indices of these two must always be the same. This is to ensure that we can map between them.
    // This is the overarching data set of all known languages. This is a strict super set of m_selectedLanguages.
    // Could have used a hash here but it'd a bit clunky since we most of the time need to access an index so
    // we'd constantly have to .values() and .keys().
    // The list of "known" languages cannot change at runtime really. They are all expected to be present all the time.
    const QStringList m_languageCodes = KLocalizedString::availableDomainTranslations("plasmashell").values();
    const LanguageVector m_languages = makeLanguages(m_languageCodes);

    // This tracks the selection and order of selected languages. It's a bit like an additional layer of model data.
    QStringList m_selectedLanguages;

    // Languages that were configured but are indeed missing from our model. These are only language codes.
    // We intentionally do not model this properly to keep things simple!
    QStringList m_missingLanguages;
};
