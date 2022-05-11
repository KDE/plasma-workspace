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

    hoverEnabled: true
    property var dayModel: null

    signal activated

    readonly property date thisDate: new Date(yearNumber, typeof monthNumber !== "undefined" ? monthNumber - 1 : 0, typeof dayNumber !== "undefined" ? dayNumber : 1)
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

    PlasmaExtras.Highlight {
        id: todayRect
        anchors.fill: parent
        hovered: true
        opacity: {
            if (today) {
                return 1;
            } else if (selected) {
                return 0.6;
            } else if (dayStyle.hovered) {
                return 0.3;
            }
            return 0;
        }
        z: -1;
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
        anchors.fill: parent

        PlasmaExtras.Heading {
            id: label
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                bottom: subDayLabel.top
            }
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: model.label || dayNumber
            opacity: isCurrent ? 1.0 : 0.5
            wrapMode: Text.NoWrap
            elide: Text.ElideRight
            fontSizeMode: Text.HorizontalFit
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
                font.pointSize: Kirigami.Theme.smallFont.pointSize
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
