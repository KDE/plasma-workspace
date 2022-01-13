/*
    optionsmodel.cpp
    SPDX-FileCopyrightText: 2021 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#include <KLocalizedString>

#include "exampleutility.cpp"
#include "formatssettings.h"
#include "optionsmodel.h"

OptionsModel::OptionsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(new FormatsSettings(this))
{
    m_staticNames = {{{i18n("Region"), QStringLiteral("lang")},
                      {i18n("Numbers"), QStringLiteral("numeric")},
                      {i18n("Time"), QStringLiteral("time")},
                      {i18n("Currency"), QStringLiteral("currency")},
                      {i18n("Measurement"), QStringLiteral("measurement")}}};
    connect(m_settings, &FormatsSettings::langChanged, this, &OptionsModel::handleLangChange);
    connect(m_settings, &FormatsSettings::numericChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(1, 0), createIndex(1, 0), {Subtitle, Example});
    });
    connect(m_settings, &FormatsSettings::timeChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(2, 0), createIndex(2, 0), {Subtitle, Example});
    });
    connect(m_settings, &FormatsSettings::monetaryChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(3, 0), createIndex(3, 0), {Subtitle, Example});
    });
    connect(m_settings, &FormatsSettings::measurementChanged, this, [this] {
        Q_EMIT dataChanged(createIndex(4, 0), createIndex(4, 0), {Subtitle, Example});
    });
}
int OptionsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_staticNames.size();
}
QVariant OptionsModel::data(const QModelIndex &index, int role) const
{
    const int row = index.row();
    if (row < 0 || row >= (int)m_staticNames.size())
        return QVariant();

    switch (role) {
    case Name:
        return m_staticNames[row].first;
    case Subtitle: {
        switch (row) {
        case 0:
            return m_settings->lang();
        case 1:
            return m_settings->numeric();
        case 2:
            return m_settings->time();
        case 3:
            return m_settings->monetary();
        case 4:
            return m_settings->measurement();
        default:
            return QVariant();
        }
    }
    case Example: {
        switch (row) {
        case 0:
            return QString();
        case 1:
            return numberExample();
        case 2:
            return timeExample();
        case 3:
            return currencyExample();
        case 4:
            return measurementExample();
        default:
            return QVariant();
        }
    }
    case Page:
        return m_staticNames[row].second;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> OptionsModel::roleNames() const
{
    return {{Name, "name"}, {Subtitle, "localeName"}, {Example, "example"}, {Page, "page"}};
}

void OptionsModel::handleLangChange()
{
    Q_EMIT dataChanged(createIndex(0, 0), createIndex(0, 0), {Subtitle, Example});

    QString defaultVal = i18n("Default");
    if (m_settings->numeric() == defaultVal) {
        Q_EMIT dataChanged(createIndex(1, 0), createIndex(1, 0), {Subtitle, Example});
    }
    if (m_settings->time() == defaultVal) {
        Q_EMIT dataChanged(createIndex(2, 0), createIndex(2, 0), {Subtitle, Example});
    }
    if (m_settings->measurement() == defaultVal) {
        Q_EMIT dataChanged(createIndex(3, 0), createIndex(3, 0), {Subtitle, Example});
    }
    if (m_settings->monetary() == defaultVal) {
        Q_EMIT dataChanged(createIndex(4, 0), createIndex(4, 0), {Subtitle, Example});
    }
}

QString OptionsModel::numberExample() const
{
    return Utility::numericExample(localeWithDefault(m_settings->numeric()));
}
QString OptionsModel::timeExample() const
{
    return Utility::timeExample(localeWithDefault(m_settings->time()));
}
QString OptionsModel::currencyExample() const
{
    return Utility::monetaryExample(localeWithDefault(m_settings->monetary()));
}
QString OptionsModel::measurementExample() const
{
    return Utility::measurementExample(localeWithDefault(m_settings->measurement()));
}
QLocale OptionsModel::localeWithDefault(const QString &val) const
{
    if (val != i18n("Default")) {
        return QLocale(val);
    } else {
        return QLocale(m_settings->lang());
    }
}
FormatsSettings *OptionsModel::settings() const
{
    return m_settings;
}
