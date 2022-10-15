/*
    optionsmodel.cpp
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QLocale>

#include <KLocalizedString>

#include "exampleutility.h"
#include "kcmregionandlang.h"
#include "optionsmodel.h"
#include "settingtype.h"

using namespace KCM_RegionAndLang;

OptionsModel::OptionsModel(KCMRegionAndLang *parent)
    : QAbstractListModel(parent)
    , m_settings(parent->settings())
{
    m_staticNames = {{{i18nc("@info:title", "Language"), SettingType::Lang},
                      {i18nc("@info:title", "Numbers"), SettingType::Numeric},
                      {i18nc("@info:title", "Time"), SettingType::Time},
                      {i18nc("@info:title", "Currency"), SettingType::Currency},
                      {i18nc("@info:title", "Measurements"), SettingType::Measurement},
                      {i18nc("@info:title", "Paper Size"), SettingType::PaperSize}}};
#ifdef LC_ADDRESS
    m_staticNames.push_back(std::make_pair(i18nc("@info:title", "Address"), SettingType::Address));
    m_staticNames.push_back(std::make_pair(i18nc("@info:title", "Name Style"), SettingType::NameStyle));
    m_staticNames.push_back(std::make_pair(i18nc("@info:title", "Phone Numbers"), SettingType::PhoneNumbers));
#endif

    connect(m_settings, &RegionAndLangSettings::langChanged, this, &OptionsModel::handleLangChange);
    connect(m_settings, &RegionAndLangSettings::numericChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::Numeric));
        m_numberExample = Utility::numericExample(locale);
        Q_EMIT dataChanged(createIndex(1, 0), createIndex(1, 0), {Subtitle, Example});
    });
    connect(m_settings, &RegionAndLangSettings::timeChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::Time));
        m_timeExample = Utility::timeExample(locale);
        Q_EMIT dataChanged(createIndex(2, 0), createIndex(2, 0), {Subtitle, Example});
    });
    connect(m_settings, &RegionAndLangSettings::monetaryChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::Currency));
        m_currencyExample = Utility::monetaryExample(locale);
        Q_EMIT dataChanged(createIndex(3, 0), createIndex(3, 0), {Subtitle, Example});
    });
    connect(m_settings, &RegionAndLangSettings::measurementChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::Measurement));
        m_measurementExample = Utility::measurementExample(locale);
        Q_EMIT dataChanged(createIndex(4, 0), createIndex(4, 0), {Subtitle, Example});
    });
    connect(m_settings, &RegionAndLangSettings::paperSizeChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::PaperSize));
        m_paperSizeExample = Utility::paperSizeExample(locale);
        Q_EMIT dataChanged(createIndex(5, 0), createIndex(5, 0), {Subtitle, Example});
    });
#ifdef LC_ADDRESS
    connect(m_settings, &RegionAndLangSettings::addressChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::Address));
        m_addressExample = Utility::addressExample(locale);
        Q_EMIT dataChanged(createIndex(6, 0), createIndex(6, 0), {Subtitle, Example});
    });
    connect(m_settings, &RegionAndLangSettings::nameStyleChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::NameStyle));
        m_nameStyleExample = Utility::nameStyleExample(locale);
        Q_EMIT dataChanged(createIndex(7, 0), createIndex(7, 0), {Subtitle, Example});
    });
    connect(m_settings, &RegionAndLangSettings::phoneNumbersChanged, this, [this] {
        QLocale locale(m_settings->LC_LocaleWithLang(SettingType::PhoneNumbers));
        m_phoneNumbersExample = Utility::phoneNumbersExample(locale);
        Q_EMIT dataChanged(createIndex(8, 0), createIndex(8, 0), {Subtitle, Example});
    });
#endif

    // initialize examples
    m_numberExample = Utility::numericExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::Numeric)));
    m_timeExample = Utility::timeExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::Time)));
    m_measurementExample = Utility::measurementExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::Measurement)));
    m_currencyExample = Utility::monetaryExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::Currency)));
    m_paperSizeExample = Utility::paperSizeExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::PaperSize)));
#ifdef LC_ADDRESS
    m_addressExample = Utility::addressExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::Address)));
    m_nameStyleExample = Utility::nameStyleExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::NameStyle)));
    m_phoneNumbersExample = Utility::phoneNumbersExample(QLocale(m_settings->LC_LocaleWithLang(SettingType::PhoneNumbers)));
#endif
}

int OptionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_staticNames.size();
}

QVariant OptionsModel::data(const QModelIndex &index, int role) const
{
    using namespace KCM_RegionAndLang;
    const int row = index.row();
    if (row < 0 || row >= (int)m_staticNames.size()) {
        return {};
    }

    switch (role) {
    case Name:
        return m_staticNames.at(row).first;
    case Subtitle: {
        switch (row) {
        case Lang:
            if (m_settings->defaultLangValue().isEmpty() && m_settings->isDefaultSetting(SettingType::Lang)) {
                // no Lang configured, no $LANG in env
                return i18nc("@info:title, the current setting is system default", "System Default");
            } else if (!m_settings->lang().isEmpty()) {
                // Lang configured and not empty
                return getNativeName(m_settings->lang());
            } else {
                // Lang configured but empty, try to read from $LANG, shouldn't happen on a valid config file
                return getNativeName(m_settings->defaultLangValue());
            }
        case Numeric:
            if (m_settings->isDefaultSetting(SettingType::Numeric)) {
                return getNativeName(m_settings->numeric());
            }
            break;
        case Time:
            if (m_settings->isDefaultSetting(SettingType::Time)) {
                return getNativeName(m_settings->time());
            }
            break;
        case Currency:
            if (m_settings->isDefaultSetting(SettingType::Currency)) {
                return getNativeName(m_settings->monetary());
            }
            break;
        case Measurement:
            if (m_settings->isDefaultSetting(SettingType::Measurement)) {
                return getNativeName(m_settings->measurement());
            }
            break;
        case PaperSize:
            if (m_settings->isDefaultSetting(SettingType::PaperSize)) {
                return getNativeName(m_settings->paperSize());
            }
            break;
#ifdef LC_ADDRESS
        case Address:
            if (m_settings->isDefaultSetting(SettingType::Address)) {
                return getNativeName(m_settings->address());
            }
            break;
        case NameStyle:
            if (m_settings->isDefaultSetting(SettingType::NameStyle)) {
                return getNativeName(m_settings->nameStyle());
            }
            break;
        case PhoneNumbers:
            if (m_settings->isDefaultSetting(SettingType::PhoneNumbers)) {
                return getNativeName(m_settings->phoneNumbers());
            }
            break;
#endif
        }
        return {}; // implicit locale
    }
    case Example: {
        switch (row) {
        case Lang:
            return {};
        case Numeric: {
            QString example = m_numberExample;
            if (m_settings->isDefaultSetting(SettingType::Numeric)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
        case Time: {
            QString example = m_timeExample;
            if (m_settings->isDefaultSetting(SettingType::Time)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
        case Currency: {
            QString example = m_currencyExample;
            if (m_settings->isDefaultSetting(SettingType::Currency)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
        case Measurement: {
            QString example = m_measurementExample;
            if (m_settings->isDefaultSetting(SettingType::Measurement)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
        case PaperSize: {
            QString example = m_paperSizeExample;
            if (m_settings->isDefaultSetting(SettingType::PaperSize)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
#ifdef LC_ADDRESS
        case Address: {
            QString example = m_addressExample;
            if (m_settings->isDefaultSetting(SettingType::Address)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
        case NameStyle: {
            QString example = m_nameStyleExample;
            if (m_settings->isDefaultSetting(SettingType::NameStyle)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
        case PhoneNumbers: {
            QString example = m_phoneNumbersExample;
            if (m_settings->isDefaultSetting(SettingType::PhoneNumbers)) {
                example += implicitFormatExampleMsg();
            }
            return example;
        }
#endif
        }
        return {};
    }
    case Page:
        return m_staticNames.at(row).second;
    }
    return {};
}

QHash<int, QByteArray> OptionsModel::roleNames() const
{
    return {{Name, QByteArrayLiteral("name")},
            {Subtitle, QByteArrayLiteral("localeName")},
            {Example, QByteArrayLiteral("example")},
            {Page, QByteArrayLiteral("page")}};
}

void OptionsModel::handleLangChange()
{
    Q_EMIT dataChanged(createIndex(0, 0), createIndex(0, 0), {Subtitle, Example});
    QLocale lang = QLocale(m_settings->lang());
    if (m_settings->isDefaultSetting(SettingType::Numeric)) {
        m_numberExample = Utility::numericExample(lang);
        Q_EMIT dataChanged(createIndex(1, 0), createIndex(1, 0), {Subtitle, Example});
    }
    if (m_settings->isDefaultSetting(SettingType::Time)) {
        m_timeExample = Utility::timeExample(lang);
        Q_EMIT dataChanged(createIndex(2, 0), createIndex(2, 0), {Subtitle, Example});
    }
    if (m_settings->isDefaultSetting(SettingType::Currency)) {
        m_currencyExample = Utility::monetaryExample(lang);
        Q_EMIT dataChanged(createIndex(3, 0), createIndex(3, 0), {Subtitle, Example});
    }
    if (m_settings->isDefaultSetting(SettingType::Measurement)) {
        m_measurementExample = Utility::measurementExample(lang);
        Q_EMIT dataChanged(createIndex(4, 0), createIndex(4, 0), {Subtitle, Example});
    }
    if (m_settings->isDefaultSetting(SettingType::PaperSize)) {
        m_paperSizeExample = Utility::paperSizeExample(lang);
        Q_EMIT dataChanged(createIndex(5, 0), createIndex(5, 0), {Subtitle, Example});
    }
#ifdef LC_ADDRESS
    if (m_settings->isDefaultSetting(SettingType::Address)) {
        m_addressExample = Utility::addressExample(lang);
        Q_EMIT dataChanged(createIndex(6, 0), createIndex(6, 0), {Subtitle, Example});
    }
    if (m_settings->isDefaultSetting(SettingType::NameStyle)) {
        m_nameStyleExample = Utility::nameStyleExample(lang);
        Q_EMIT dataChanged(createIndex(7, 0), createIndex(7, 0), {Subtitle, Example});
    }
    if (m_settings->isDefaultSetting(SettingType::PhoneNumbers)) {
        m_phoneNumbersExample = Utility::phoneNumbersExample(lang);
        Q_EMIT dataChanged(createIndex(8, 0), createIndex(8, 0), {Subtitle, Example});
    }
#endif
}

QString OptionsModel::implicitFormatExampleMsg() const
{
    QString locale;

    if (!m_settings->lang().isEmpty()) {
        locale = getNativeName(m_settings->lang());
    } else if (!m_settings->defaultLangValue().isEmpty()) {
        locale = getNativeName(m_settings->defaultLangValue());
    } else {
        locale = i18nc("@info:title, the current setting is system default", "System Default");
    }
    return i18nc("as subtitle, remind user that the format used now is inherited from locale %1", " (Standard format for %1)", locale);
}

QString OptionsModel::getNativeName(const QString &locale) const
{
    return QLocale(locale).nativeLanguageName();
}
