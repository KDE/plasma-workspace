/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include "../colorcorrectconstants.h"

#include <QObject>
#include <QTime>
#include <QVariant>

using namespace ColorCorrect;

class kwin_dbus : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kwin.ColorCorrect")

public:
    kwin_dbus();
    ~kwin_dbus() override;

    bool registerDBus();
    bool unregisterDBus();

    bool nightColorAvailable = true;

    bool activeEnabled = true;
    bool active = true;

    bool modeEnabled = true;
    int mode = 0;

    bool nightTemperatureEnabled = true;
    int nightTemperature = DEFAULT_NIGHT_TEMPERATURE;

    bool running = false;
    int currentColorTemperature = NEUTRAL_TEMPERATURE;

    double latitudeAuto = 0;
    double longitudeAuto = 0;

    bool locationEnabled = true;
    double latitudeFixed = 0;
    double longitudeFixed = 0;

    bool timingsEnabled = true;
    QTime morningBeginFixed = QTime(6, 0, 0);
    QTime eveningBeginFixed = QTime(18, 0, 0);
    int transitionTime = FALLBACK_SLOW_UPDATE_TIME;

    bool configChangeExpectSuccess;

public Q_SLOTS:
    QHash<QString, QVariant> nightColorInfo();
    bool setNightColorConfig(QHash<QString, QVariant> data);
    void nightColorAutoLocationUpdate(double latitude, double longitude);

Q_SIGNALS:
    void nightColorConfigChanged(QHash<QString, QVariant> data);

private:
    QHash<QString, QVariant> getData();
};
