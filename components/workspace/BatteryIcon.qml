/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    property bool hasBattery
    property int percent
    property bool pluggedIn
    property string batteryType

    PlasmaCore.Svg {
        id: svg
        imagePath: "icons/battery"
        colorGroup: PlasmaCore.ColorScope.colorGroup
        onRepaintNeeded: { // needed to detect the hint item go away when theme changes
            batterySvg.visible = Qt.binding(() => !otherBatteriesSvg.visible && (!svg.hasElement("hint-dont-superimpose-fill") || !hasBattery))
        }
    }

    PlasmaCore.SvgItem {
        id: batterySvg
        anchors.centerIn: parent
        width: PlasmaCore.Units.roundToIconSize(Math.min(parent.width, parent.height))
        height: width
        svg: svg
        elementId: "Battery"
        visible: !otherBatteriesSvg.visible && (!svg.hasElement("hint-dont-superimpose-fill") || !hasBattery)
    }

    PlasmaCore.SvgItem {
        id: fillSvg
        anchors.fill: batterySvg
        svg: svg
        elementId: hasBattery ? fillElement(percent) : "Unavailable"
        visible: !otherBatteriesSvg.visible
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
            } else {
                return "Fill0";
            }
        } else {
            if (p >= 95) {
                return "Fill100";
            } else if (p >= 85) {
                return "Fill90";
            } else if (p >= 75) {
                return "Fill80";
            } else if (p >= 65) {
                return "Fill70";
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
            } else {
                return "Fill0";
            }
        }
    }

    PlasmaCore.SvgItem {
        anchors.fill: batterySvg
        svg: svg
        elementId: "AcAdapter"
        visible: pluggedIn && !otherBatteriesSvg.visible
    }

    PlasmaCore.IconItem {
        id: otherBatteriesSvg
        anchors.fill: batterySvg
        source: elementForType(batteryType)
        visible: source !== ""
    }

    function elementForType(t) {
        switch(t) {
            case "Mouse":
                return "input-mouse-battery";
            case "Keyboard":
                return "input-keyboard-battery";
            case "Pda":
                return "phone-battery";
            case "Phone":
                return "phone-battery";
            case "Ups":
                return "battery-ups";
            case "GamingInput":
                return "input-gaming-battery";
            case "Bluetooth":
                return "preferences-system-bluetooth-battery";
            default:
                return "";
        }
    }
}
