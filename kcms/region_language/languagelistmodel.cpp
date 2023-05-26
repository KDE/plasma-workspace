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
    /* explicitly set pt to pt_PT as a workaround for GNU Gettext and CLDR treat the default dialect of 'pt' differently
     *
     * see https://invent.kde.org/plasma/plasma-workspace/-/merge_requests/2478
     * and https://mail.kde.org/pipermail/kde-i18n-doc/2023-January/001340.html
     * for more info on the matter
     */
    auto availableLanguages = KLocalizedString::availableDomainTranslations("plasmashell");
    if (availableLanguages.contains(QStringLiteral("pt"))) {
        availableLanguages.remove(QStringLiteral("pt"));
        availableLanguages.insert(QStringLiteral("pt_PT"));
    }

    m_availableLanguages = availableLanguages.values();
    m_availableLanguages.sort();
    m_availableLanguages.push_front(QStringLiteral("C"));
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
        return languageCodeToName(m_availableLanguages.at(row));
    case LanguageCode:
        return m_availableLanguages.at(row);
    case Flag: {
        QString flagCode;
        const QStringList split = QLocale(m_availableLanguages.at(row)).name().split(QLatin1Char('_'));
        if (split.size() > 1) {
            flagCode = split[1].toLower();
        }
        return QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf5/locale/countries/%1/flag.png").arg(flagCode));
    }
    }
    Q_UNREACHABLE();
    return {};
}

QHash<int, QByteArray> LanguageListModel::roleNames() const
{
    return {{NativeName, QByteArrayLiteral("nativeName")}, {LanguageCode, QByteArrayLiteral("languageCode")}, {Flag, QByteArrayLiteral("flag")}};
}

QString LanguageListModel::languageCodeToName(const QString &languageCode)
{
    const QLocale locale(languageCode);
    const QString languageName = locale.nativeLanguageName();

    if (languageName.isEmpty()) {
        return languageCode;
    }

    if (languageCode.contains(QLatin1Char('@'))) {
        return i18nc("%1 is language name, %2 is language code name", "%1 (%2)", languageName, languageCode);
    }

    // KDE languageCode got translated by QLocale to a locale code we also have on
    // the list. Currently this only happens with pt that gets translated to pt_BR.
    if (languageCode == QStringLiteral("pt_BR")) {
        return i18nc("%1 is português in system locale name, Brazil is to distinguish European português and Brazilian português", "%1 (Brazil)", languageName);
    }

    return languageName;
}

bool LanguageListModel::isSupportedLanguage(const QString &language) const
{
    return m_availableLanguages.contains(language);
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
    if (m_settings->language().isEmpty()) {
        // no language but have lang
        m_selectedLanguages = {m_settings->lang()};
    } else {
        // have language, ignore lang
        m_selectedLanguages = m_settings->language().split(QLatin1Char(':'));
    }

    if (m_settings->isDefaultSetting(SettingType::Lang) && m_settings->isDefaultSetting(SettingType::Language)) {
        m_hasImplicitLang = true;
        Q_EMIT hasImplicitLangChanged();
    }

    endResetModel();

    // check for invalid lang
    if (!m_selectedLanguages.empty() && !m_parent->isSupportedLanguage(m_selectedLanguages.front())) {
        m_unsupportedLanguage = m_selectedLanguages.front();
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

    return LanguageListModel::languageCodeToName(m_selectedLanguages.at(row));
}

bool SelectedLanguageModel::shouldWarnMultipleLang() const
{
    if (m_selectedLanguages.size() >= 2) {
        if (m_selectedLanguages.front() == QStringLiteral("en_US")) {
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
    if (lang.isEmpty() || m_selectedLanguages.indexOf(lang) != -1) {
        return;
    }

    // fix Kirigami.SwipeListItem doesn't connect to Actions' visible property.
    // Reset model enforce a refresh of delegate
    beginResetModel();
    if (m_hasImplicitLang) {
        m_hasImplicitLang = false;
        Q_EMIT hasImplicitLangChanged();
    }
    m_selectedLanguages.push_back(lang);
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

    int existLangIndex = m_selectedLanguages.indexOf(lang);
    // return if no change, but allow change implicit lang to explicit
    if (existLangIndex == index && !m_hasImplicitLang) {
        return;
    }

    beginResetModel();
    m_selectedLanguages[index] = lang;
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
        if (!m_parent->isSupportedLanguage(m_selectedLanguages.front())) {
            m_unsupportedLanguage = m_selectedLanguages.front();
            Q_EMIT unsupportedLanguageChanged();
        } else {
            if (!m_unsupportedLanguage.isEmpty()) {
                m_unsupportedLanguage.clear();
                Q_EMIT unsupportedLanguageChanged();
            }

            auto glibcLang = m_kcm->toGlibcLocale(m_selectedLanguages.front());
            if (glibcLang.has_value()) {
                m_settings->setLang(glibcLang.value());
            }
        }
        QString languages;
        for (auto i = m_selectedLanguages.cbegin(); i != m_selectedLanguages.cend(); i++) {
            languages.push_back(*i);
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
