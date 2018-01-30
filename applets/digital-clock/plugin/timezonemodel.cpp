/***************************************************************************
 *   Copyright (C) 2014 Kai Uwe Broulik <kde@privat.broulik.de>            *
 *   Copyright (C) 2014  Martin Klapetek <mklapetek@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "timezonemodel.h"
#include "timezonesi18n.h"

#include <QTimeZone>
#include <QStringMatcher>
#include <KLocalizedString>

TimeZoneFilterProxy::TimeZoneFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    m_stringMatcher.setCaseSensitivity(Qt::CaseInsensitive);
}

bool TimeZoneFilterProxy::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!sourceModel() || m_filterString.isEmpty()) {
        return true;
    }

    const QString city = sourceModel()->index(source_row, 0, source_parent).data(TimeZoneModel::CityRole).toString();
    const QString region = sourceModel()->index(source_row, 0, source_parent).data(TimeZoneModel::RegionRole).toString();
    const QString comment = sourceModel()->index(source_row, 0, source_parent).data(TimeZoneModel::CommentRole).toString();

    if (m_stringMatcher.indexIn(city) != -1 || m_stringMatcher.indexIn(region) != -1 ||
        m_stringMatcher.indexIn(comment) != -1) {
        return true;
    }

    return false;
}

void TimeZoneFilterProxy::setFilterString(const QString &filterString)
{
    m_filterString = filterString;
    m_stringMatcher.setPattern(filterString);
    emit filterStringChanged();
    invalidateFilter();
}

//=============================================================================

TimeZoneModel::TimeZoneModel(QObject *parent)
    : QAbstractListModel(parent),
      m_timezonesI18n(new TimezonesI18n(this))
{
    update();
}

TimeZoneModel::~TimeZoneModel()
{
}

int TimeZoneModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.count();
}

QVariant TimeZoneModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        TimeZoneData currentData = m_data.at(index.row());

        switch(role) {
        case TimeZoneIdRole:
            return currentData.id;
        case RegionRole:
            return currentData.region;
        case CityRole:
            return currentData.city;
        case CommentRole:
            return currentData.comment;
        case CheckedRole:
            return currentData.checked;
        }
    }

    return QVariant();
}

bool TimeZoneModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || value.isNull()) {
        return false;
    }

    if (role == CheckedRole) {
        m_data[index.row()].checked = value.toBool();
        emit dataChanged(index, index);

        if (m_data[index.row()].checked) {
            m_selectedTimeZones.append(m_data[index.row()].id);
            m_offsetData.insert(m_data[index.row()].id, m_data[index.row()].offsetFromUtc);
        } else {
            m_selectedTimeZones.removeAll(m_data[index.row()].id);
            m_offsetData.remove(m_data[index.row()].id);
        }

        sortTimeZones();

        emit selectedTimeZonesChanged();
        return true;
    }

    return false;
}

void TimeZoneModel::update()
{
    beginResetModel();
    m_data.clear();

    QTimeZone localZone = QTimeZone(QTimeZone::systemTimeZoneId());
    const QStringList data = QString::fromUtf8(localZone.id()).split(QLatin1Char('/'));

    TimeZoneData local;
    local.id = QStringLiteral("Local");
    local.region = i18nc("This means \"Local Timezone\"", "Local");
    local.city = m_timezonesI18n->i18nCity(data.last());
    local.comment = i18n("Your system time zone");
    local.checked = false;

    m_data.append(local);

    QStringList cities;
    QHash<QString, QTimeZone> zonesByCity;

    const QList<QByteArray> systemTimeZones = QTimeZone::availableTimeZoneIds();

    for (auto it = systemTimeZones.constBegin(); it != systemTimeZones.constEnd(); ++it) {
        const QTimeZone zone(*it);
        const QStringList splitted = QString::fromUtf8(zone.id()).split(QStringLiteral("/"));

        // CITY | COUNTRY | CONTINENT
        const QString key = QStringLiteral("%1|%2|%3").arg(splitted.last(),
                                                    QLocale::countryToString(zone.country()),
                                                    splitted.first());

        cities.append(key);
        zonesByCity.insert(key, zone);
    }
    cities.sort(Qt::CaseInsensitive);

    Q_FOREACH (const QString &key, cities) {
        const QTimeZone timeZone = zonesByCity.value(key);
        QString comment = timeZone.comment();

        if (!comment.isEmpty()) {
            comment = i18n(comment.toUtf8());
        }

        const QStringList cityCountryContinent = key.split(QLatin1Char('|'));

        TimeZoneData newData;
        newData.id = timeZone.id();
        newData.region = timeZone.country() == QLocale::AnyCountry ? QString()
                                                                   : m_timezonesI18n->i18nContinents(cityCountryContinent.at(2)) + QLatin1Char('/') + m_timezonesI18n->i18nCountry(timeZone.country());
        newData.city = m_timezonesI18n->i18nCity(cityCountryContinent.at(0));
        newData.comment = comment;
        newData.checked = false;
        newData.offsetFromUtc = timeZone.offsetFromUtc(QDateTime::currentDateTimeUtc());
        m_data.append(newData);
    }

    endResetModel();
}

void TimeZoneModel::setSelectedTimeZones(const QStringList &selectedTimeZones)
{
    m_selectedTimeZones = selectedTimeZones;
    for (int i = 0; i < m_data.size(); i++) {
        if (m_selectedTimeZones.contains(m_data.at(i).id)) {
            m_data[i].checked = true;
            m_offsetData.insert(m_data[i].id, m_data[i].offsetFromUtc);

            QModelIndex index = createIndex(i, 0);
            emit dataChanged(index, index);
        }
    }

    sortTimeZones();
}

void TimeZoneModel::selectLocalTimeZone()
{
    m_data[0].checked = true;

    QModelIndex index = createIndex(0, 0);
    emit dataChanged(index, index);

    m_selectedTimeZones << m_data[0].id;
    emit selectedTimeZonesChanged();
}

QHash<int, QByteArray> TimeZoneModel::roleNames() const
{
    return QHash<int, QByteArray>({
        {TimeZoneIdRole, "timeZoneId"},
        {RegionRole, "region"},
        {CityRole, "city"},
        {CommentRole, "comment"},
        {CheckedRole, "checked"}
    });
}

void TimeZoneModel::sortTimeZones()
{
    std::sort(m_selectedTimeZones.begin(), m_selectedTimeZones.end(),
              [this](const QString &a, const QString &b) {
                  return m_offsetData.value(a) < m_offsetData.value(b);
              });
}
