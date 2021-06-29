/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
    Sun *sun();
    Moon *moon();

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
