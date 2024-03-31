/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents
import org.kde.plasma.plasmoid
import org.kde.kirigami as Kirigami

Item {
    id: tooltipContentItem

    property int preferredTextWidth: Kirigami.Units.gridUnit * 20

    implicitWidth: mainLayout.implicitWidth + Kirigami.Units.gridUnit
    implicitHeight: mainLayout.implicitHeight + Kirigami.Units.gridUnit

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    /**
     * These accessible properties are used in the compact representation,
     * not here.
     */
    Accessible.name: i18nc("@info:tooltip %1 is a localized long date", "Today is %1", tooltipSubtext.text)
    Accessible.description: {
        const description = tooltipSubLabelText.visible ? [tooltipSubLabelText.text] : [];
        for (let i = 0; i < timezoneRepeater.count; i += 2) {
            description.push(`${timezoneRepeater.itemAt(i).text}: ${timezoneRepeater.itemAt(i + 1).text}`);
        }
        return description.join('; ');
    }

    ColumnLayout {
        id: mainLayout

        anchors {
            left: parent.left
            top: parent.top
            margins: Kirigami.Units.largeSpacing
        }

        spacing: 0

        Kirigami.Heading {
            id: tooltipMaintext

            Layout.minimumWidth: Math.min(implicitWidth, tooltipContentItem.preferredTextWidth)
            Layout.maximumWidth: tooltipContentItem.preferredTextWidth

            level: 3
            elide: Text.ElideRight
            // keep this consistent with toolTipMainText in analog-clock
            text: clocks.visible ? Qt.formatDate(root.tzDate, Qt.locale(), Locale.LongFormat) : Qt.locale().toString(root.tzDate, "dddd")
            textFormat: Text.PlainText
        }

        PlasmaComponents.Label {
            id: tooltipSubtext

            Layout.minimumWidth: Math.min(implicitWidth, tooltipContentItem.preferredTextWidth)
            Layout.maximumWidth: tooltipContentItem.preferredTextWidth

            text: {
                if (Plasmoid.configuration.showSeconds === 0) {
                    return Qt.formatDate(root.tzDate, Qt.locale(), root.dateFormatString);
                } else {
                    return "%1\n%2"
                        .arg(Qt.formatTime(root.tzDate, Qt.locale(), Locale.LongFormat))
                        .arg(Qt.formatDate(root.tzDate, Qt.locale(), root.dateFormatString))
                }
            }
            opacity: 0.6
            visible: !clocks.visible
        }

        PlasmaComponents.Label {
            id: tooltipSubLabelText
            Layout.minimumWidth: Math.min(implicitWidth, tooltipContentItem.preferredTextWidth)
            Layout.maximumWidth: tooltipContentItem.preferredTextWidth
            text: (root.fullRepresentationItem as CalendarView)?.monthView.todayAuxilliaryText ?? ""
            textFormat: Text.PlainText
            opacity: 0.6
            visible: !clocks.visible && text.length > 0
        }

        GridLayout {
            id: clocks

            Layout.minimumWidth: Math.min(implicitWidth, tooltipContentItem.preferredTextWidth)
            Layout.maximumWidth: tooltipContentItem.preferredTextWidth
            Layout.minimumHeight: childrenRect.height
            visible: timezoneRepeater.count > 2
            columns: 2
            rowSpacing: 0

            Repeater {
                id: timezoneRepeater

                model: root.selectedTimeZonesDeduplicatingExplicitLocalTimeZone()
                    // Duplicate each entry, because that's how we do "tables" with 2 columns in QML. :-\
                    // An alternative would be a nested Repeater with an ObjectModel.
                    .reduce((array, item) => {
                        array.push(item, item);
                        return array;
                    }, [])

                PlasmaComponents.Label {
                    // Layout.fillWidth is buggy here
                    Layout.alignment: index % 2 === 0 ? Qt.AlignRight : Qt.AlignLeft
                    text: {
                        if (index % 2 === 0) {
                            return i18nc("@label %1 is a city or time zone name", "%1:", root.displayStringForTimeZone(modelData));
                        } else {
                            return timeForZone(modelData, Plasmoid.configuration.showSeconds > 0);
                        }
                    }
                    textFormat: Text.PlainText
                    font.weight: root.timeZoneResolvesToLastSelectedTimeZone(modelData) ? Font.Bold : Font.Normal
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                }
            }
        }
    }
}
