/*
 *   Copyright 2009 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef TIMESOURCE_H
#define TIMESOURCE_H

#include <Plasma/DataContainer>
#include <QTimeZone>

class Sun;
class Moon;

class TimeSource : public Plasma::DataContainer
{
    Q_OBJECT

public:
    explicit TimeSource(const QString &name, QObject *parent = nullptr);
    ~TimeSource() override;
    void setTimeZone(const QString &name);
    void updateTime();

private:
    QString parseName(const QString &name);
    void addMoonPositionData(const QDateTime &dt);
    void addDailyMoonPositionData(const QDateTime &dt);
    void addSolarPositionData(const QDateTime &dt);
    void addDailySolarPositionData(const QDateTime &dt);
    Sun* sun();
    Moon* moon();

    QString m_tzName;
    int m_offset;
    double m_latitude;
    double m_longitude;
    Sun *m_sun;
    Moon *m_moon;
    bool m_moonPosition : 1;
    bool m_solarPosition : 1;
    bool m_userDateTime : 1;
    bool m_local : 1;
    QTimeZone m_tz;
};

#endif

