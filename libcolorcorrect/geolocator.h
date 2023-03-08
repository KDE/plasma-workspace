/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "colorcorrect_export.h"

#include <Plasma5Support/DataEngine>
#include <Plasma5Support/DataEngineConsumer>

namespace Plasma5Support
{
class DataEngine;
}

namespace ColorCorrect
{
class COLORCORRECT_EXPORT Geolocator : public QObject, public Plasma5Support::DataEngineConsumer
{
    Q_OBJECT

    Q_PROPERTY(double latitude READ latitude NOTIFY latitudeChanged)
    Q_PROPERTY(double longitude READ longitude NOTIFY longitudeChanged)

public:
    explicit Geolocator(QObject *parent = nullptr);
    ~Geolocator() override = default;

    double latitude() const
    {
        return m_latitude;
    }
    double longitude() const
    {
        return m_longitude;
    }

public Q_SLOTS:
    /**
     * Called when data is updated by Plasma::DataEngine
     */
    void dataUpdated(const QString &source, const Plasma5Support::DataEngine::Data &data);

Q_SIGNALS:
    void locationChanged(double latitude, double longitude);
    void latitudeChanged();
    void longitudeChanged();

private:
    Plasma5Support::DataEngine *m_engine = nullptr;

    double m_latitude = 0;
    double m_longitude = 0;
};

}
