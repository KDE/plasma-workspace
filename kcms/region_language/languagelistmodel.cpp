/*
    languagelistmodel.h
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>
    SPDX-FileCopyrightText: 2019 Kevin Ottens <kevin.ottens@enioka.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "languagelistmodel.h"
#include "exampleutility.h"
#include "kcm_regionandlang_debug.h"
#include "kcmregionandlang.h"
#include "regionandlangsettings.h"

using namespace KCM_RegionAndLang;

LanguageListModel::LanguageListModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_selectedLanguageModel(new SelectedLanguageModel(this))
{
    connect(this, &LanguageListModel::isPreviewExampleChanged, this, &LanguageListModel::exampleChanged);
    connect(m_selectedLanguageModel, &SelectedLanguageModel::exampleChanged, this, &LanguageListModel::exampleChanged);

    const auto availableLanguages = KLocalizedString::availableDomainTranslations("plasmashell");
    for (const QString &availableLanguage : availableLanguages) {
        /* explicitly set pt to pt_PT as a workaround for GNU Gettext and CLDR treat the default dialect of 'pt' differently
         *
         * see https://invent.kde.org/plasma/plasma-workspace/-/merge_requests/2478
         * and https://mail.kde.org/pipermail/kde-i18n-doc/2023-January/001340.html
         * for more info on the matter
         */
        if (availableLanguage.contains(QStringLiteral("pt"))) {
            m_availableLanguages.emplace_back("pt_PT");
        } else {
            m_availableLanguages.emplace_back(availableLanguage);
        }
    }

    std::sort(m_availableLanguages.begin(), m_availableLanguages.end(), [](const QLocale &a, const QLocale &b) {
        return a.name() > b.name();
    });
}

int LanguageListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_availableLanguages.size();
}

QVariant LanguageListModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();
    if (row < 0 || row >= m_availableLanguages.size()) {
        return {};
    }
    switch (role) {
    case NativeName:
        return getLocaleName(m_availableLanguages.at(row));
    case LanguageCode:
        return m_availableLanguages.at(row).name();
    case Flag: {
        QString flagCode;
        const QStringList split = QLocale(m_availableLanguages.at(row)).name().split(QLatin1Char('_'));
        if (split.size() > 1) {
            flagCode = split[1].toLower();
        }
        return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf6/locale/countries/%1/flag.png").arg(flagCode));
    }
    }
    Q_UNREACHABLE();
    return {};
}

QHash<int, QByteArray> LanguageListModel::roleNames() const
{
    return {{NativeName, QByteArrayLiteral("nativeName")}, {LanguageCode, QByteArrayLiteral("languageCode")}, {Flag, QByteArrayLiteral("flag")}};
}

QString LanguageListModel::getLocaleName(const QLocale &locale)
{
    const QString languageName = locale.nativeLanguageName();

    if (languageName.isEmpty()) {
        return locale.name();
    }

    if (locale.name().contains(QLatin1Char('@'))) {
        return i18nc("%1 is language name, %2 is language code name", "%1 (%2)", languageName, locale.name());
    }

    // KDE languageCode got translated by QLocale to a locale code we also have on
    // the list. Currently this only happens with pt that gets translated to pt_BR.
    if (locale.name() == QStringLiteral("pt_BR")) {
        return i18nc("%1 is português in system locale name, Brazil is to distinguish European português and Brazilian português", "%1 (Brazil)", languageName);
    }

    return languageName;
}

bool LanguageListModel::isSupportedLocale(const QLocale &locale) const
{
    return m_availableLanguages.contains(locale);
}

int LanguageListModel::currentIndex() const
{
    return m_index;
}

void LanguageListModel::setCurrentIndex(int index)
{
    if (index == m_index || index < 0 || index >= m_availableLanguages.size()) {
        return;
    }

    m_index = index;
    Q_EMIT exampleChanged();
}

QString LanguageListModel::exampleHelper(const std::function<QString(const QLocale &)> &func) const
{
    if (!m_settings) {
        return {};
    }

    QString result = func(QLocale(m_settings->langWithFallback()));
    if (m_isPreviewExample) {
        if (m_index < 0) {
            result = func(QLocale(m_settings->langWithFallback()));
        } else {
            result = func(QLocale(m_availableLanguages[m_index]));
        }
    }
    return result;
}

QString LanguageListModel::numberExample() const
{
    return exampleHelper(Utility::numericExample);
}

QString LanguageListModel::currencyExample() const
{
    return exampleHelper(Utility::monetaryExample);
}

QString LanguageListModel::timeExample() const
{
    return exampleHelper(Utility::timeExample);
}

QString LanguageListModel::paperSizeExample() const
{
    return exampleHelper(Utility::paperSizeExample);
}

#ifdef LC_ADDRESS
QString LanguageListModel::addressExample() const
{
    return exampleHelper(Utility::addressExample);
}

QString LanguageListModel::nameStyleExample() const
{
    return exampleHelper(Utility::nameStyleExample);
}

QString LanguageListModel::phoneNumbersExample() const
{
    return exampleHelper(Utility::phoneNumbersExample);
}
#endif

QString LanguageListModel::metric() const
{
    return exampleHelper(Utility::measurementExample);
}

void LanguageListModel::setRegionAndLangSettings(QObject *settings, QObject *kcm)
{
    if (auto *regionandlangsettings = qobject_cast<RegionAndLangSettings *>(settings)) {
        if (auto *regionandlangkcm = qobject_cast<KCMRegionAndLang *>(kcm)) {
            m_settings = regionandlangsettings;
            m_selectedLanguageModel->setRegionAndLangSettings(regionandlangsettings, regionandlangkcm);
            Q_EMIT exampleChanged();
        }
    }
}

bool LanguageListModel::isPreviewExample() const
{
    return m_isPreviewExample;
}

void LanguageListModel::setIsPreviewExample(bool preview)
{
    m_isPreviewExample = preview;
}

SelectedLanguageModel::SelectedLanguageModel(LanguageListModel *parent)
    : QAbstractListModel(parent)
    , m_parent(parent)
{
}

void SelectedLanguageModel::setRegionAndLangSettings(RegionAndLangSettings *settings, KCMRegionAndLang *kcm)
{
    m_settings = settings;
    m_kcm = kcm;

    beginResetModel();

    const QStringList languages = m_settings->language().split(':');
    for (const QString &language : languages) {
        m_selectedLanguages.push_back(QLocale(language));
    }

    endResetModel();

    // check for invalid lang
    if (!m_selectedLanguages.empty() && !m_parent->isSupportedLocale(m_selectedLanguages.front())) {
        m_unsupportedLanguage = m_selectedLanguages.front().name();
        Q_EMIT unsupportedLanguageChanged();
    } else if (!m_unsupportedLanguage.isEmpty()) {
        m_unsupportedLanguage.clear();
        Q_EMIT unsupportedLanguageChanged();
    }
}

SelectedLanguageModel *LanguageListModel::selectedLanguageModel() const
{
    return m_selectedLanguageModel;
}

int SelectedLanguageModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_selectedLanguages.size();
}

QVariant SelectedLanguageModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role)
    const auto row = index.row();
    if (row < 0 || row > m_selectedLanguages.size()) {
        return {};
    }
    // "add Language" Item
    if (row == m_selectedLanguages.size()) {
        return {};
    }

    return LanguageListModel::getLocaleName(m_selectedLanguages.at(row));
}

bool SelectedLanguageModel::shouldWarnMultipleLang() const
{
    if (m_selectedLanguages.size() >= 2) {
        if (m_selectedLanguages.front().name().startsWith(QLatin1String("en_"))) {
            return true;
        }
    }
    return false;
}

void SelectedLanguageModel::move(int from, int to)
{
    if (from == to || from < 0 || from >= m_selectedLanguages.size() || to < 0 || to >= m_selectedLanguages.size()) {
        return;
    }

    if (m_hasImplicitLang) {
        m_hasImplicitLang = false;
        Q_EMIT hasImplicitLangChanged();
    }

    beginResetModel();
    m_selectedLanguages.move(from, to);
    endResetModel();
    saveLanguages();
    Q_EMIT shouldWarnMultipleLangChanged();
    Q_EMIT exampleChanged();
}

void SelectedLanguageModel::remove(int index)
{
    if (index < 0 || index >= m_selectedLanguages.size()) {
        return;
    }
    beginRemoveRows(QModelIndex(), index, index);
    m_selectedLanguages.removeAt(index);
    endRemoveRows();
    saveLanguages();
    Q_EMIT shouldWarnMultipleLangChanged();
    Q_EMIT exampleChanged();
}

void SelectedLanguageModel::addLanguage(const QString &lang)
{
    QLocale locale(lang);

    if (lang.isEmpty() || m_selectedLanguages.indexOf(locale)) {
        return;
    }

    // fix Kirigami.SwipeListItem doesn't connect to Actions' visible property.
    // Reset model enforce a refresh of delegate
    beginResetModel();
    if (m_hasImplicitLang) {
        m_hasImplicitLang = false;
        Q_EMIT hasImplicitLangChanged();
    }
    m_selectedLanguages.push_back(locale);
    endResetModel();
    saveLanguages();
    Q_EMIT shouldWarnMultipleLangChanged();
    Q_EMIT exampleChanged();
}

void SelectedLanguageModel::replaceLanguage(int index, const QString &lang)
{
    if (index < 0 || index >= m_selectedLanguages.size() || lang.isEmpty()) {
        return;
    }

    QLocale locale(lang);
    int existLangIndex = m_selectedLanguages.indexOf(locale);
    // return if no change, but allow change implicit lang to explicit
    if (existLangIndex == index && !m_hasImplicitLang) {
        return;
    }

    beginResetModel();
    m_selectedLanguages[index] = locale;
    if (!m_hasImplicitLang) {
        // delete duplicate lang
        if (existLangIndex != -1) {
            m_selectedLanguages.removeAt(existLangIndex);
        }
    } else {
        m_hasImplicitLang = false;
        Q_EMIT hasImplicitLangChanged();
    }
    endResetModel();
    saveLanguages();
    Q_EMIT shouldWarnMultipleLangChanged();
    Q_EMIT exampleChanged();
}

void SelectedLanguageModel::saveLanguages()
{
    // implicit lang means no change
    if (!m_settings || m_hasImplicitLang) {
        return;
    }
    if (m_selectedLanguages.empty()) {
        m_settings->setLang(m_settings->defaultLangValue());
        m_settings->config()->group(QStringLiteral("Formats")).deleteEntry("lang");
        m_settings->config()->group(QStringLiteral("Translations")).deleteEntry("language");
    } else {
        if (!m_parent->isSupportedLocale(m_selectedLanguages.front())) {
            m_unsupportedLanguage = m_selectedLanguages.front().name();
            Q_EMIT unsupportedLanguageChanged();
        } else {
            if (!m_unsupportedLanguage.isEmpty()) {
                m_unsupportedLanguage.clear();
                Q_EMIT unsupportedLanguageChanged();
            }

            auto glibcLang = m_kcm->toGlibcLocale(m_selectedLanguages.front().name());
            if (glibcLang.has_value()) {
                m_settings->setLang(glibcLang.value());
            }
        }
        QString languages;
        for (auto i = m_selectedLanguages.cbegin(); i != m_selectedLanguages.cend(); i++) {
            languages.push_back(i->name());
            // no ':' at end
            if (i + 1 != m_selectedLanguages.cend()) {
                languages.push_back(QLatin1Char(':'));
            }
        }
        m_settings->setLanguage(languages);
    }
}

bool SelectedLanguageModel::hasImplicitLang() const
{
    return m_hasImplicitLang;
}

const QString &SelectedLanguageModel::unsupportedLanguage() const
{
    return m_unsupportedLanguage;
}

QString SelectedLanguageModel::envLang() const
{
    return qEnvironmentVariable("LANG");
}

QString SelectedLanguageModel::envLanguage() const
{
    return qEnvironmentVariable("LANGUAGE");
}