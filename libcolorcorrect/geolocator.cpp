/********************************************************************
Copyright 2017 Roman Gilg <subdiff@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "geolocator.h"
#include <cmath>

namespace ColorCorrect
{
Geolocator::Geolocator(QObject *parent)
    : QObject(parent)
{
    m_engine = dataEngine(QStringLiteral("geolocation"));

    if (m_engine && m_engine->isValid()) {
        m_engine->connectSource(QStringLiteral("location"), this);
    }
}

void Geolocator::dataUpdated(const QString &source, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(source);

    if (!m_engine) {
        return;
    }

    if (data.isEmpty()) {
        return;
    }

    auto readGeoDouble = [](const Plasma::DataEngine::Data &data, const QString &key) -> double {
        const Plasma::DataEngine::Data::ConstIterator it = data.find(key);

        if (it == data.end()) {
            return qQNaN();
        }

        bool ok = false;
        double result = it.value().toDouble(&ok);
        if (!ok) {
            result = qQNaN();
        }
        return result;
    };

    double lat = readGeoDouble(data, QStringLiteral("latitude"));
    double lng = readGeoDouble(data, QStringLiteral("longitude"));
    if (std::isnan(lat) || std::isnan(lng)) {
        return;
    }

    bool isChanged = false;
    if (m_latitude != lat) {
        m_latitude = lat;
        isChanged = true;
        emit latitudeChanged();
    }
    if (m_longitude != lng) {
        m_longitude = lng;
        isChanged = true;
        emit longitudeChanged();
    }
    if (isChanged) {
        emit locationChanged(m_latitude, m_longitude);
    }
}

}
