/*
    SPDX-FileCopyrightText: 2009 Petri Damstén <damu@iki.fi>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <gps.h>

#include "geolocationprovider.h"

class Gpsd : public QThread
{
    Q_OBJECT
public:
    explicit Gpsd(gps_data_t *gpsdata);
    ~Gpsd() override;

    void update();

Q_SIGNALS:
    void dataReady(const Plasma::DataEngine::Data &data);

protected:
    void run() override;

private:
    gps_data_t *m_gpsdata;
    QMutex m_mutex;
    QWaitCondition m_condition;
    bool m_abort;
};

class Gps : public GeolocationProvider
{
    Q_OBJECT
public:
    explicit Gps(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    ~Gps() override;

    void update() override;

private:
    Gpsd *m_gpsd;
#if GPSD_API_MAJOR_VERSION >= 5
    gps_data_t *m_gpsdata;
#endif
};
