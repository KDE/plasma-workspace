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
    return QHash<QString, QVariant> {
        { QStringLiteral("Available"), nightColorAvailable },

        { QStringLiteral("ActiveEnabled"), activeEnabled },
        { QStringLiteral("Active"), active },

        { QStringLiteral("ModeEnabled"), modeEnabled },
        { QStringLiteral("Mode"), mode },

        { QStringLiteral("NightTemperatureEnabled"), nightTemperatureEnabled },
        { QStringLiteral("NightTemperature"), nightTemperature },

        { QStringLiteral("Running"), running },
        { QStringLiteral("CurrentColorTemperature"), currentColorTemperature },

        { QStringLiteral("LatitudeAuto"), latitudeAuto },
        { QStringLiteral("LongitudeAuto"), longitudeAuto },

        { QStringLiteral("LocationEnabled"), locationEnabled },
        { QStringLiteral("LatitudeFixed"), latitudeFixed },
        { QStringLiteral("LongitudeFixed"), longitudeFixed },

        { QStringLiteral("TimingsEnabled"), timingsEnabled },
        { QStringLiteral("MorningBeginFixed"), morningBeginFixed.toString(Qt::ISODate) },
        { QStringLiteral("EveningBeginFixed"), eveningBeginFixed.toString(Qt::ISODate) },
        { QStringLiteral("TransitionTime"), transitionTime },
    };
}

bool kwin_dbus::setNightColorConfig(QHash<QString, QVariant> data)
{
    if (!configChangeExpectSuccess) {
        return false;
    }

    bool hasChanged = false;

    if (data.contains("Active")) {
        auto val = data["Active"].toBool();
        hasChanged |= active != val;
        active = val;
    }
    if (data.contains("Mode")) {
        auto val = data["Mode"].toInt();
        hasChanged |= mode != val;
        mode = val;
    }
    if (data.contains("NightTemperature")) {
        auto val = data["NightTemperature"].toInt();
        hasChanged |= nightTemperature != val;
        nightTemperature = val;
    }
    if (data.contains("LatitudeFixed")) {
        auto valLat = data["LatitudeFixed"].toDouble();
        auto valLng = data["LongitudeFixed"].toDouble();
        hasChanged |= latitudeFixed != valLat || longitudeFixed != valLng;
        latitudeFixed = valLat;
        longitudeFixed = valLng;
    }
    if (data.contains("MorningBeginFixed")) {
        auto valMbf = QTime::fromString(data["MorningBeginFixed"].toString(), Qt::ISODate);
        auto valEbf = QTime::fromString(data["EveningBeginFixed"].toString(), Qt::ISODate);
        auto valTrT = data["TransitionTime"].toInt();
        hasChanged |= morningBeginFixed != valMbf || eveningBeginFixed != valEbf
                        || transitionTime != valTrT;
        morningBeginFixed = valMbf;
        eveningBeginFixed = valEbf;
        transitionTime = valTrT;
    }
    running = active;

    if (hasChanged) {
        emit nightColorConfigChanged(getData());
    }
    return true;
}

void kwin_dbus::nightColorAutoLocationUpdate(double latitude, double longitude)
{
    latitudeAuto = latitude;
    longitudeAuto = longitude;

    emit nightColorConfigChanged(getData());
}
