/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
        Q_EMIT latitudeChanged();
    }
    if (m_longitude != lng) {
        m_longitude = lng;
        isChanged = true;
        Q_EMIT longitudeChanged();
    }
    if (isChanged) {
        Q_EMIT locationChanged(m_latitude, m_longitude);
    }
}

}
