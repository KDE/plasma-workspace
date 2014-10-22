/*
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *   Copyright (C) 2014 by Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

import QtQuick 2.2
import QtQuick.Layouts 1.1
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.workspace.components 2.0 as PW

ColumnLayout {
   BreezeLabel { //should be a heading but we want it _loads_ bigger
        text: Qt.formatTime(timeSource.data["Local"]["DateTime"])
        //we fill the width then align the text so that we can make the text shrink to fit
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignRight

        font.weight: Font.DemiBold
        fontSizeMode: Text.HorizontalFit
        font.pointSize: 36
    }

    BreezeLabel {
        text: Qt.formatDate(timeSource.data["Local"]["DateTime"], Qt.DefaultLocaleLongDate);
        Layout.alignment: Qt.AlignRight
    }

    RowLayout {
        Layout.alignment: Qt.AlignRight
        visible: pmSource.connectedSources != "" && pmSource.data["Battery"]["Has Battery"] && pmSource.data["Battery0"] != undefined && pmSource.data["Battery0"]["Is Power Supply"]

        PW.BatteryIcon {
            id: battery
            hasBattery: true
            percent: pmSource.data["Battery0"] ? pmSource.data["Battery0"]["Percent"] : 0
            pluggedIn: pmSource.data["Battery0"] ? pmSource.data["Battery0"]["State"] != "Discharging" : false

            height: batteryLabel.height
            width: batteryLabel.height
        }

        BreezeLabel {
            id: batteryLabel
            text: {
                var state = pmSource.data["Battery0"] ? pmSource.data["Battery0"]["State"] : "";
                switch(state) {
                case "NoCharge": //follow through
                case "Discharging":
                    return i18nd("plasma_lookandfeel_org.kde.lookandfeel","%1% battery remaining", battery.percent)
                case "FullyCharged":
                    return i18nd("plasma_lookandfeel_org.kde.lookandfeel","Fully charged")
                default:
                    return i18nd("plasma_lookandfeel_org.kde.lookandfeel","%1%. Charging", battery.percent)
                }
            }
            Layout.alignment: Qt.AlignRight
            wrapMode: Text.Wrap
        }
    }

    
    PlasmaCore.DataSource {
        id: pmSource
        engine: "powermanagement"
        connectedSources: sources
        onSourceAdded: {
            if (source == "Battery0") {
                disconnectSource(source);
                connectSource(source);
            }
        }
        onSourceRemoved: {
            if (source == "Battery0") {
                disconnectSource(source);
            }
        }
    }

    PlasmaCore.DataSource {
        id: timeSource
        engine: "time"
        connectedSources: ["Local"]
        interval: 1000
    }

}
