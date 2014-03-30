/*
 *   Copyright (C) 2009 Petri Damstén <damu@iki.fi>
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GPS_H
#define GPS_H

#include <gps.h>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "geolocationprovider.h"

class Gpsd : public QThread
{
    Q_OBJECT
public:
    Gpsd(gps_data_t* gpsdata);
    virtual ~Gpsd();

    void update();

Q_SIGNALS:
    void dataReady(const Plasma::DataEngine::Data& data);

protected:
    virtual void run();

private:
    gps_data_t* m_gpsdata;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_abort;
};

class Gps : public GeolocationProvider
{
    Q_OBJECT
public:
    explicit Gps(QObject *parent = 0, const QVariantList &args = QVariantList());
    ~Gps();

    void update();

private:
    Gpsd* m_gpsd;
#if GPSD_API_MAJOR_VERSION >= 5
    gps_data_t* m_gpsdata;
#endif
};

#endif
