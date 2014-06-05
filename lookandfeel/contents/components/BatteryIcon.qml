/*
 *   Copyright 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *   Copyright 2013 Kai Uwe Broulik <kde@privat.broulik.de>
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

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    property bool hasBattery
    property int percent
    property bool pluggedIn

    PlasmaCore.Svg {
        id: svg
        imagePath: "icons/battery"
    }

    PlasmaCore.SvgItem {
        id: batterySvg
        anchors.fill: parent
        svg: svg
        elementId: "Battery"
    }

    PlasmaCore.SvgItem {
        id: fillSvg
        anchors.fill: parent
        svg: svg
        elementId: hasBattery ? fillElement(percent) : "Unavailable"
        visible: elementId != ""
    }

    function fillElement(p) {
        // We switched from having steps of 20 for the battery percentage to a more accurate
        // step of 10. This means we break other and older themes.
        // If the Fill10 element is not found, it is likely that the theme doesn't support
        // that and we use the older method of obtaining the fill element.
        if (!svg.hasElement("Fill10")) {
            print("No Fill10 element found in your theme's battery.svg - Using legacy 20% steps for battery icon");
            if (p >= 90) {
                return "Fill100";
            } else if (p >= 70) {
                return "Fill80";
            } else if (p >= 50) {
                return "Fill60";
            } else if (p > 20) {
                return "Fill40";
            } else if (p >= 10) {
                return "Fill20";
            }
            return "";
        } else {
            if (p >= 95) {
                return "Fill100";
            } else if (p >= 85) {
                return "Fill90";
            } else if (p >= 75) {
                return "Fill90";
            } else if (p >= 65) {
                return "Fill80";
            } else if (p >= 55) {
                return "Fill60";
            } else if (p >= 45) {
                return "Fill50";
            } else if (p >= 35) {
                return "Fill40";
            } else if (p >= 25) {
                return "Fill30";
            } else if (p >= 15) {
                return "Fill20";
            } else if (p > 5) {
                return "Fill10";
            }
            return "";
        }
    }

    PlasmaCore.SvgItem {
        anchors.fill: parent
        svg: svg
        elementId: "AcAdapter"
        visible: pluggedIn
    }
}
