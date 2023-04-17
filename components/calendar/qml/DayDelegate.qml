/*
    SPDX-FileCopyrightText: 2013 Heena Mahour <heena393@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2015 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2021 Jan Blackquill <uhhadd@gmail.com>
    SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import QtQml 2.15 // For Date
import QtQml.Models 2.15
import org.kde.kirigami 2.12 as Kirigami

import org.kde.plasma.workspace.calendar 2.0

PlasmaComponents3.AbstractButton {
    id: dayStyle

    objectName: {
        switch (dateMatchingPrecision) {
        case Calendar.MatchYear:
            return "calendarCell-" + yearNumber;
        case Calendar.MatchYearAndMonth:
            return "calendarCell-" + yearNumber + "-" + monthNumber;
        case Calendar.MatchYearMonthAndDay:
        default:
            return "calendarCell-" + yearNumber + "-" + monthNumber + "-" + dayNumber;
        }
    }
    hoverEnabled: true
    property var dayModel: null

    signal activated

    readonly property date thisDate: new Date(yearNumber, typeof monthNumber !== "undefined" ? monthNumber - 1 : 0, typeof dayNumber !== "undefined" ? dayNumber : 1)

    Accessible.name: thisDate.toLocaleDateString(Qt.locale(), Locale.LongFormat)
    Accessible.description: {
        const eventDescription = (model.eventCount !== undefined && model.eventCount > 0) ? i18ndp("plasmashellprivateplugin", "%1 event", "%1 events", model.eventCount) : i18nd("plasmashellprivateplugin", "No events");
        const subLabelDescription = model.subLabel || model.subDayLabel || "";
        return `${eventDescription} ${subLabelDescription ? `; ${subLabelDescription}` : ""}`;
    }

    readonly property bool today: {
        const today = root.today;
        let result = true;
        if (dateMatchingPrecision >= Calendar.MatchYear) {
            result = result && today.getFullYear() === thisDate.getFullYear()
        }
        if (dateMatchingPrecision >= Calendar.MatchYearAndMonth) {
            result = result && today.getMonth() === thisDate.getMonth()
        }
        if (dateMatchingPrecision >= Calendar.MatchYearMonthAndDay) {
            result = result && today.getDate() === thisDate.getDate()
        }
        return result
    }
    readonly property bool selected: {
        const current = root.currentDate;
        let result = true;
        if (dateMatchingPrecision >= Calendar.MatchYear) {
            result = result && current.getFullYear() === thisDate.getFullYear()
        }
        if (dateMatchingPrecision >= Calendar.MatchYearAndMonth) {
            result = result && current.getMonth() === thisDate.getMonth()
        }
        if (dateMatchingPrecision >= Calendar.MatchYearMonthAndDay) {
            result = result && current.getDate() === thisDate.getDate()
        }
        return result
    }

    Loader {
        anchors.fill: parent

        active: dayStyle.activeFocus
        asynchronous: true

        sourceComponent: PlasmaCore.FrameSvgItem {
            anchors {
                leftMargin: -margins.left
                topMargin: -margins.top
                rightMargin: -margins.right
                bottomMargin: -margins.bottom
            }
            imagePath: "widgets/button"
            prefix: ["toolbutton-focus", "focus"]
        }
    }

    Loader {
        anchors.fill: parent

        active: today || selected || dayStyle.hovered || dayStyle.activeFocus
        asynchronous: true
        z: -1

        sourceComponent: PlasmaExtras.Highlight {
            hovered: true
            opacity: {
                if (today) {
                    return 1;
                } else if (selected) {
                    return 0.6;
                } else if (dayStyle.hovered) {
                    return 0.3;
                } else if (dayStyle.activeFocus) {
                    return 0.1;
                }
                return 0;
            }
        }
    }

    Loader {
        active: model.eventCount !== undefined && model.eventCount > 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: subDayLabel.item ? subDayLabel.item.implicitHeight
            : PlasmaCore.Units.smallSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        sourceComponent: Row {
            spacing: PlasmaCore.Units.smallSpacing

            property bool hasSubDayLabel: false

            Repeater {
                model: DelegateModel {
                    model: dayStyle.dayModel
                    delegate: Rectangle {
                        width: PlasmaCore.Units.smallSpacing * (hasSubDayLabel ? 1 : 1.5)
                        height: width
                        radius: width / 2
                        color: model.eventColor ? Kirigami.ColorUtils.linearInterpolation(model.eventColor, PlasmaCore.Theme.textColor, 0.2) : PlasmaCore.Theme.highlightColor
                    }

                    Component.onCompleted: rootIndex = modelIndex(index)
                }
            }
        }

        onLoaded: item.hasSubDayLabel = Qt.binding(() => subDayLabel.active)
    }

    contentItem: Item {
        // ColumnLayout makes scrolling too slow, so use anchors to position labels

        PlasmaExtras.Heading {
            id: label
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: subDayLabel.top
            }
            font.pixelSize: Math.max(
                PlasmaCore.Theme.defaultFont.pixelSize * 1.35 /* Level 1 Heading */,
                daysCalendar.cellHeight / (daysCalendar.dateMatchingPrecision === Calendar.MatchYearMonthAndDay ? 3 /* weeksColumn */ : 6))
            font.pointSize: -1 // Avoid QML warnings
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: model.label || dayNumber
            opacity: isCurrent ? 1.0 : 0.5
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
        }

        Loader {
            id: subDayLabel
            active: (!!model.subDayLabel && model.subDayLabel.length > 0)
                 || typeof(model.alternateDayNumber) === "number"
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }

            sourceComponent: PlasmaComponents3.Label {
                elide: Text.ElideRight
                font.pixelSize: Math.max(
                    PlasmaCore.Theme.smallestFont.pixelSize,
                    daysCalendar.cellHeight / (daysCalendar.dateMatchingPrecision === Calendar.MatchYearMonthAndDay ? 6 : 12))
                font.pointSize: -1 // Avoid QML warnings
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                maximumLineCount: 1
                opacity: label.opacity
                // Prefer sublabel over day number
                text: model.subDayLabel || model.alternateDayNumber.toString()
                textFormat: Text.PlainText
                wrapMode: Text.NoWrap
            }
        }

        Loader {
            id: tooltipLoader
            active: !!model.subLabel

            sourceComponent: PlasmaComponents3.ToolTip {
                visible: Kirigami.Settings.isMobile? dayStyle.pressed : dayStyle.hovered
                text: model.subLabel
            }

            onLoaded: {
                if (dayStyle.today) {
                    root.todayAuxilliaryText = model.subLabel;
                }
            }
        }
    }
}
