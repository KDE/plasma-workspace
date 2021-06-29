/*
    SPDX-FileCopyrightText: 2014 John Layt <john@layt.net>
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "translationsmodel.h"

#include <QDebug>
#include <QLocale>
#include <QMetaEnum>
#include <QMetaObject>

#include "config-workspace.h"
#include "debug.h"

#ifdef HAVE_PACKAGEKIT
    // Ubuntu completion depends on packagekit. When packagekit is not available there's no point supporting
    // completion checking as we'll have no way to complete the language if it is incomplete.
#endif
QHash<int, QByteArray> TranslationsModel::roleNames() const
{
    QHash<int, QByteArray> roles = QAbstractItemModel::roleNames();

    const auto e = QMetaEnum::fromType<AdditionalRoles>();
    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }

    return roles;
}

QVariant TranslationsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_languages.count()) {
        return QVariant();
    }

    Language *const lang = m_languages.at(index.row());
    const QString &code = lang->code;

    if (role == Qt::DisplayRole) {
        return languageCodeToName(code);
    }

    switch (static_cast<AdditionalRoles>(role)) {
    case Object:
        return QVariant::fromValue(lang);
    case LanguageCode:
        return code;
    case IsIncomplete:
        return lang->state == Language::State::Incomplete;
    case IsInstalling:
        return lang->state == Language::State::Installing;
    case IsSelected:
        return m_selectedLanguages.contains(code);
    case SelectionPreference:
        return m_selectedLanguages.indexOf(code);
    }

    return {};
}

int TranslationsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_languages.count();
}

QString TranslationsModel::languageCodeToName(const QString &languageCode) const
{
    const QLocale locale(languageCode);
    const QString &languageName = locale.nativeLanguageName();

    if (languageName.isEmpty()) {
        return languageCode;
    }

    if (languageCode.contains(QLatin1Char('@'))) {
        return i18nc("%1 is language name, %2 is language code name", "%1 (%2)", languageName, languageCode);
    }

    if (locale.name() != languageCode && m_languageCodes.contains(locale.name())) {
        // KDE languageCode got translated by QLocale to a locale code we also have on
        // the list. Currently this only happens with pt that gets translated to pt_BR.
        if (languageCode == QLatin1String("pt")) {
            return QLocale(QStringLiteral("pt_PT")).nativeLanguageName();
        }

        qCWarning(KCM_TRANSLATIONS) << "Language code morphed into another existing language code, please report!" << languageCode << locale.name();
        return i18nc("%1 is language name, %2 is language code name", "%1 (%2)", languageName, languageCode);
    }

    return languageName;
}

QStringList TranslationsModel::selectedLanguages() const
{
    return m_selectedLanguages;
}

void TranslationsModel::setSelectedLanguages(const QStringList &languages)
{
    if (m_selectedLanguages == languages) {
        return;
    }

    m_selectedLanguages = languages;
    Q_EMIT selectedLanguagesChanged(languages);

    QStringList missing;
    for (const QString &code : languages) {
        if (const int index = m_languageCodes.indexOf(code); index >= 0) {
            Language *const lang = m_languages.at(index);
            lang->reloadCompleteness();
            const QModelIndex modelIndex = createIndex(index, 0);
            Q_EMIT dataChanged(modelIndex, modelIndex, {IsSelected, SelectionPreference});
        } else {
            missing << code;
        }
    }

    missing.sort();
    if (m_missingLanguages != missing) {
        m_missingLanguages = missing;
        Q_EMIT missingLanguagesChanged();
    }
}

QStringList TranslationsModel::missingLanguages() const
{
    return m_missingLanguages;
}

void TranslationsModel::move(int from, int to)
{
    if (from >= m_selectedLanguages.count() || to >= m_selectedLanguages.count()) {
        return;
    }

    if (from == to) {
        return;
    }

    // Reset the entire model. Figuring out what moved where is hardly worth the effort as
    // only very few languages will be selected and thus visible when a move occurs.
    beginResetModel();
    m_selectedLanguages.move(from, to);
    Q_EMIT selectedLanguagesChanged(m_selectedLanguages);
    endResetModel();
}

void TranslationsModel::remove(const QString &languageCode)
{
    if (languageCode.isEmpty()) {
        return;
    }

    const int index = m_languageCodes.indexOf(languageCode);
    if (index < 0 || m_selectedLanguages.count() < 2) {
        return;
    }
    const QModelIndex modelIndex = createIndex(index, 0);

    m_selectedLanguages.removeAll(languageCode);
    Q_EMIT selectedLanguagesChanged(m_selectedLanguages);
    Q_EMIT dataChanged(modelIndex, modelIndex, {IsSelected, SelectionPreference});
}

LanguageVector TranslationsModel::makeLanguages(const QStringList &codes)
{
    LanguageVector ret;
    for (const auto &code : codes) {
        auto lang = new Language(code, this);
        connect(lang, &Language::stateChanged, this, [this, lang] {
            const int index = m_languages.indexOf(lang);
            if (index < 0) {
                qCWarning(KCM_TRANSLATIONS) << "Failed to find index for " << lang->code;
                return;
            }

            const QModelIndex modelIndex = createIndex(index, 0);
            Q_EMIT dataChanged(modelIndex, modelIndex, {IsInstalling, IsIncomplete});
        });
        ret.push_back(lang);
    }
    return ret;
}

#include "translationsmodel.moc"
