/*
 *  Copyright (C) 2014 John Layt <john@layt.net>
 *  Copyright (C) 2018 Eike Hein <hein@kde.org>
 *  Copyright (C) 2021 Harald Sitter <sitter@kde.org>
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

#ifndef TRANSLATIONSMODEL_H
#define TRANSLATIONSMODEL_H

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

#endif
