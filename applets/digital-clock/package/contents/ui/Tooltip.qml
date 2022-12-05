/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami

Item {
    id: tooltipContentItem

    property int preferredTextWidth: PlasmaCore.Units.gridUnit * 20

    implicitWidth: mainLayout.implicitWidth + PlasmaCore.Units.gridUnit
    implicitHeight: mainLayout.implicitHeight + PlasmaCore.Units.gridUnit

    LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
    LayoutMirroring.childrenInherit: true
    PlasmaCore.ColorScope.colorGroup: PlasmaCore.Theme.NormalColorGroup
    PlasmaCore.ColorScope.inherit: false

    /**
     * These accessible properties are used in the compact representation,
     * not here.
     */
    Accessible.name: i18nc("@info:tooltip %1 is a localized long date", "Today is %1", tooltipSubtext.text)
    Accessible.description: {
        let description = [];
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
            margins: PlasmaCore.Units.gridUnit / 2
        }

        spacing: 0

        PlasmaExtras.Heading {
            id: tooltipMaintext
            level: 3
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            elide: Text.ElideRight
            text: clocks.visible ? Qt.formatDate(tzDate, Locale.LongFormat) : Qt.formatDate(tzDate,"dddd")
        }

        PlasmaComponents3.Label {
            id: tooltipSubtext
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            text: Qt.formatDate(tzDate, dateFormatString)
            opacity: 0.6
            visible: !clocks.visible
        }

        GridLayout {
            id: clocks
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            Layout.minimumHeight: childrenRect.height
            visible: timezoneRepeater.count > 1
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
                    text: index % 2 == 0 ? i18nc("@label %1 is a city or time zone name", "%1:", nameForZone(modelData)) : timeForZone(modelData)
                    font.weight: modelData === Plasmoid.configuration.lastSelectedTimezone ? Font.Bold : Font.Normal
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone
                }
            }
        }
    }
}
