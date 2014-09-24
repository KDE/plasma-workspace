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

#include <QTimeZone>
#include <KLocalizedString>
#include <QDebug>

TimeZoneModel::TimeZoneModel(QObject *parent) : QAbstractListModel(parent)
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
        } else {
            m_selectedTimeZones.removeAll(m_data[index.row()].id);
        }

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
    QStringList data = QString::fromUtf8(localZone.id()).split(QLatin1Char('/'));

    TimeZoneData local;
    local.id = QStringLiteral("Local");
    local.region = i18nc("This means \"Local Timezone\"", "Local");
    local.city = data.at(1);
    local.comment = i18n("Your system time zone");
    local.checked = false;

    m_data.append(local);

    QStringList cities;
    QHash<QString, QTimeZone> zonesByCity;

    QList<QByteArray> systemTimeZones = QTimeZone::availableTimeZoneIds();

    for (auto it = systemTimeZones.constBegin(); it != systemTimeZones.constEnd(); ++it) {
        const QTimeZone zone(*it);
        const QString continentCity = zone.id();
        const int separator = continentCity.lastIndexOf('/');

        // CITY | COUNTRY | CONTINENT
        const QString key = QString("%1|%2|%3").arg(continentCity.mid(separator + 1),
                                                    QLocale::countryToString(zone.country()),
                                                    continentCity.left(separator));

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

        QStringList cityCountryContinent = key.split(QLatin1Char('|'));

        TimeZoneData newData;
        newData.id = timeZone.id();
        newData.region = cityCountryContinent.at(2) + QLatin1Char('/') + cityCountryContinent.at(1);
        newData.city = cityCountryContinent.at(0);
        newData.comment = comment;
        newData.checked = false;
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

            QModelIndex index = createIndex(i, 0);
            emit dataChanged(index, index);
        }
    }
}

void TimeZoneModel::selectLocalTimeZone()
{
    m_data[0].checked = true;

    QModelIndex index = createIndex(0, 0);
    emit dataChanged(index, index);

    m_selectedTimeZones << m_data[0].id;
    emit selectedTimeZonesChanged();
}

QHash<int, QByteArray> TimeZoneModel::roleNames() const {
    return QHash<int, QByteArray>({
        {TimeZoneIdRole, "timeZoneId"},
        {RegionRole, "region"},
        {CityRole, "city"},
        {CommentRole, "comment"},
        {CheckedRole, "checked"}
    });
}
