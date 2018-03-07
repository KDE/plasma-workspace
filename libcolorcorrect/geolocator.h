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
#ifndef COLORCORRECT_GEOLOCATOR_H
#define COLORCORRECT_GEOLOCATOR_H

#include "colorcorrect_export.h"

#include <Plasma/DataEngine>
#include <Plasma/DataEngineConsumer>

namespace Plasma
{
class DataEngine;
}

namespace ColorCorrect
{

class COLORCORRECT_EXPORT Geolocator : public QObject, public Plasma::DataEngineConsumer
{
    Q_OBJECT

    Q_PROPERTY(double latitude READ latitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude NOTIFY longitudeChanged)

public:
    explicit Geolocator(QObject *parent = nullptr);
    ~Geolocator() override = default;

    double latitude() const {
        return m_latitude;
    }
    double longitude() const {
        return m_longitude;
    }

public Q_SLOTS:
    /**
     * Called when data is updated by Plasma::DataEngine
     */
    void dataUpdated(const QString &source, const Plasma::DataEngine::Data &data);

Q_SIGNALS:
    void locationChanged(double latitude, double longitude);
    void latitudeChanged();
    void longitudeChanged();

private:
    Plasma::DataEngine *m_engine = nullptr;

    double m_latitude = 0;
    double m_longitude = 0;
};

}

#endif // COLORCORRECT_GEOLOCATOR_H
