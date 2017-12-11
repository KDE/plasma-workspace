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
#include "mock_kwin.h"

#include <QDBusConnection>

using namespace ColorCorrect;

kwin_dbus::kwin_dbus()
{
}

kwin_dbus::~kwin_dbus()
{
}

bool kwin_dbus::registerDBus()
{
    bool ret;
    QDBusConnection dbus = QDBusConnection::sessionBus();
    ret = dbus.registerObject("/ColorCorrect", this, QDBusConnection::ExportAllContents);
    ret &= dbus.registerService("org.kde.KWin");
    return ret;
}

bool kwin_dbus::unregisterDBus()
{
    bool ret;
    QDBusConnection dbus = QDBusConnection::sessionBus();
    ret = dbus.unregisterService("org.kde.KWin");
    dbus.unregisterObject("/ColorCorrect");
    return ret;
}

QHash<QString, QVariant> kwin_dbus::nightColorInfo()
{
    return getData();
}

QHash<QString, QVariant> kwin_dbus::getData()
{
    QHash<QString, QVariant> ret;
    ret["Available"] = nightColorAvailable;

    ret["ActiveEnabled"] = activeEnabled;
    ret["Active"] = active;

    ret["ModeEnabled"] = modeEnabled;
    ret["Mode"] = mode;

    ret["NightTemperatureEnabled"] = nightTemperatureEnabled;
    ret["NightTemperature"] = nightTemperature;

    ret["Running"] = running;
    ret["CurrentColorTemperature"] = currentColorTemperature;

    ret["LatitudeAuto"] = latitudeAuto;
    ret["LongitudeAuto"] = longitudeAuto;

    ret["LocationEnabled"] = locationEnabled;
    ret["LatitudeFixed"] = latitudeFixed;
    ret["LongitudeFixed"] = longitudeFixed;

    ret["TimingsEnabled"] = timingsEnabled;
    ret["MorningBeginFixed"] = morningBeginFixed.toString(Qt::ISODate);
    ret["EveningBeginFixed"] = eveningBeginFixed.toString(Qt::ISODate);
    ret["TransitionTime"] = transitionTime;

    return ret;
}

bool kwin_dbus::nightColorConfigChange(QHash<QString, QVariant> data)
{
    if (!configChangeExpectSuccess) {
        return false;
    }

    if (data.contains("Active")) {
        active = data["Active"].toBool();
    }
    if (data.contains("Mode")) {
        mode = data["Mode"].toInt();
    }
    if (data.contains("NightTemperature")) {
        nightTemperature = data["NightTemperature"].toInt();
    }
    if (data.contains("LatitudeFixed")) {
        latitudeFixed = data["LatitudeFixed"].toDouble();
        longitudeFixed = data["LongitudeFixed"].toDouble();
    }
    if (data.contains("MorningBeginFixed")) {
        morningBeginFixed = QTime::fromString(data["MorningBeginFixed"].toString(), Qt::ISODate);
        eveningBeginFixed = QTime::fromString(data["EveningBeginFixed"].toString(), Qt::ISODate);
        transitionTime = data["TransitionTime"].toInt();
    }
    running = active;

    emit nightColorConfigChangeSignal(getData());
    return true;
}

void kwin_dbus::nightColorAutoLocationUpdate(double latitude, double longitude)
{
    latitudeAuto = latitude;
    longitudeAuto = longitude;

    emit nightColorConfigChangeSignal(getData());
}
