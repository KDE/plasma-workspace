/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQml
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami

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
        let description = tooltipSubLabelText.visible ? [tooltipSubLabelText.text] : [];
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

            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth

            level: 3
            elide: Text.ElideRight
            // keep this consistent with toolTipMainText in analog-clock
            text: clocks.visible ? Qt.formatDate(tzDate, Qt.locale(), Locale.LongFormat) : Qt.formatDate(tzDate,"dddd")
            textFormat: Text.PlainText
        }

        PlasmaComponents3.Label {
            id: tooltipSubtext

            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth

            text: {
                if (Plasmoid.configuration.showSeconds === 0) {
                    return Qt.formatDate(tzDate, Qt.locale(), dateFormatString);
                } else {
                    return "%1\n%2"
                        .arg(Qt.formatTime(tzDate, Qt.locale(), Locale.LongFormat))
                        .arg(Qt.formatDate(tzDate, Qt.locale(), dateFormatString))
                }
            }
            opacity: 0.6
            visible: !clocks.visible
        }

        PlasmaComponents3.Label {
            id: tooltipSubLabelText
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            text: root.fullRepresentationItem ? root.fullRepresentationItem.monthView.todayAuxilliaryText : ""
            textFormat: Text.PlainText
            opacity: 0.6
            visible: !clocks.visible && text.length > 0
        }

        GridLayout {
            id: clocks

            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            Layout.minimumHeight: childrenRect.height
            visible: timezoneRepeater.count > 2
            columns: 2
            rowSpacing: 0

            Repeater {
                id: timezoneRepeater

                model: {
                    let timezones = [];
                    for (let i = 0; i < Plasmoid.configuration.selectedTimeZones.length; i++) {
                        let thisTzData = Plasmoid.configuration.selectedTimeZones[i];

                        /* Don't add this item if it's the same as the local time zone, which
                         * would indicate that the user has deliberately added a dedicated entry
                         * for the city of their normal time zone. This is not an error condition
                         * because the user may have done this on purpose so that their normal
                         * local time zone shows up automatically while they're traveling and
                         * they've switched the current local time zone to something else. But
                         * with this use case, when they're back in their normal local time zone,
                         * the clocks list would show two entries for the same city. To avoid
                         * this, let's suppress the duplicate.
                         */
                        if (!(thisTzData !== "Local" && nameForZone(thisTzData) === nameForZone("Local"))) {
                            timezones.push(thisTzData);
                            timezones.push(thisTzData);
                        }
                    }

                    return timezones;
                }

                PlasmaComponents3.Label {
                    // Layout.fillWidth is buggy here
                    Layout.alignment: index % 2 === 0 ? Qt.AlignRight : Qt.AlignLeft
                    text: {
                        if (index % 2 === 0) {
                            return i18nc("@label %1 is a city or time zone name", "%1:", nameForZone(modelData));
                        } else {
                            return timeForZone(modelData, Plasmoid.configuration.showSeconds > 0);
                        }
                    }
                    textFormat: Text.PlainText
                    font.weight: modelData === Plasmoid.configuration.lastSelectedTimezone ? Font.Bold : Font.Normal
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                }
            }
        }
    }
}
