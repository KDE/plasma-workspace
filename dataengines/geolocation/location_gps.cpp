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

#include "location_gps.h"
#include "geolocdebug.h"

Gpsd::Gpsd(gps_data_t* gpsdata)
    : m_gpsdata(gpsdata)
    , m_abort(false)
{
}

Gpsd::~Gpsd()
{
    m_abort = true;
    m_condition.wakeOne();
    wait();
}

void Gpsd::update()
{
    if (!isRunning()) {
        start();
    } else {
        m_condition.wakeOne();
    }
}

void Gpsd::run()
{
#if defined( GPSD_API_MAJOR_VERSION ) && ( GPSD_API_MAJOR_VERSION >= 3 ) && defined( WATCH_ENABLE )
    gps_stream(m_gpsdata, WATCH_ENABLE, nullptr);
#else
    gps_query(m_gpsdata, "w+x\n");
#endif

    while (!m_abort) {
        Plasma::DataEngine::Data d;

#if GPSD_API_MAJOR_VERSION >= 7
        if (gps_read(m_gpsdata, NULL, 0) != -1) {
#elif GPSD_API_MAJOR_VERSION >= 5
	if (gps_read(m_gpsdata) != -1) {
#else
        if (gps_poll(m_gpsdata) != -1) {
#endif
            //qDebug() << "poll ok";
#if GPSD_API_MAJOR_VERSION >= 9
            if (m_gpsdata->online.tv_sec || m_gpsdata->online.tv_nsec) {
#else
            if (m_gpsdata->online) {
#endif
                //qDebug() << "online";
#if GPSD_API_MAJOR_VERSION >= 10
                if (m_gpsdata->fix.status != STATUS_NO_FIX) {
#else
                if (m_gpsdata->status != STATUS_NO_FIX) {
#endif
                    //qDebug() << "fix";
                    d["accuracy"] = 30;
                    d["latitude"] = QString::number(m_gpsdata->fix.latitude);
                    d["longitude"] = QString::number(m_gpsdata->fix.longitude);
                }
            }
        }

        emit dataReady(d);

        m_condition.wait(&m_mutex);
    }
}

Gps::Gps(QObject* parent, const QVariantList& args)
    : GeolocationProvider(parent, args),
      m_gpsd(nullptr)
#if GPSD_API_MAJOR_VERSION >= 5
    , m_gpsdata(nullptr)
#endif
{
#if GPSD_API_MAJOR_VERSION >= 5
    m_gpsdata = new gps_data_t;
    if (gps_open("localhost", DEFAULT_GPSD_PORT, m_gpsdata) != -1) {
#else
    gps_data_t* m_gpsdata = gps_open("localhost", DEFAULT_GPSD_PORT);
    if (m_gpsdata) {
#endif
        qCDebug(DATAENGINE_GEOLOCATION)<< "gpsd found.";
        m_gpsd = new Gpsd(m_gpsdata);
        connect(m_gpsd, SIGNAL(dataReady(Plasma::DataEngine::Data)),
                this, SLOT(setData(Plasma::DataEngine::Data)));
    } else {
        qCWarning(DATAENGINE_GEOLOCATION) << "gpsd not found";
    }

    setIsAvailable(m_gpsd);
}

Gps::~Gps()
{
    delete m_gpsd;
#if GPSD_API_MAJOR_VERSION >= 5
    delete m_gpsdata;
#endif
}

void Gps::update()
{
    if (m_gpsd) {
        m_gpsd->update();
    }
}

K_EXPORT_PLASMA_GEOLOCATIONPROVIDER(gps, Gps)

#include "location_gps.moc"
