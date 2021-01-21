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
#ifndef COLORCORRECT_AUTOTESTS_MOCK_KWIN_H
#define COLORCORRECT_AUTOTESTS_MOCK_KWIN_H

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

#endif // COLORCORRECT_AUTOTESTS_MOCK_KWIN_H
