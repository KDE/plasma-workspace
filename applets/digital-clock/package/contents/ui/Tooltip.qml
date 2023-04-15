/*
    SPDX-FileCopyrightText: 2015 Martin Klapetek <mklapetek@kde.org>
    SPDX-FileCopyrightText: 2023 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQml
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.plasmoid 2.0
import org.kde.kirigami 2.20 as Kirigami
import org.kde.plasma.clock 1.0


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

    // just share with DC?
    Clock {
        id: clock
        timeZone: Plasmoid.configuration.selectedTimeZones[Plasmoid.configuration.lastSelectedTimezone]
        trackSeconds: Plasmoid.configuration.showSeconds
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
            // keep this consistent with toolTipMainText in analog-clock
            text: clocks.visible ? Qt.formatDate(clock.dateTime, Locale.LongFormat) : Qt.formatDate(clock.dateTime,"dddd")
        }

        PlasmaComponents3.Label {
            id: tooltipSubtext
            Layout.minimumWidth: Math.min(implicitWidth, preferredTextWidth)
            Layout.maximumWidth: preferredTextWidth
            text: Plasmoid.configuration.showSeconds === 0 ? Qt.formatDate(clock.dateTime, dateFormatString)
                                                           : Qt.formatTime(clock.dateTime, Qt.locale().timeFormat(Locale.LongFormat))
                                                             + "\n"
                                                             + Qt.formatDate(clock.dateTime, Qt.formatDate(clock.dateTime, dateFormatString))
            opacity: 0.6
            visible: !clocks.visible
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
                        //  */

                        if (thisTzData !== "Local") {
                            timezones.push(thisTzData);
                        }
                    }
                    return timezones;
                }

                PlasmaComponents3.Label {
                    // Layout.fillWidth is buggy here
                    Layout.alignment: index % 2 === 0 ? Qt.AlignRight : Qt.AlignLeft
                    text: index % 2 == 0 ? i18nc("@label %1 is a city or time zone name", "%1:", clock.timeZoneName) : formatTime(clock.dateTime, Plasmoid.configuration.showSeconds > 0)
                    font.weight: modelData === Plasmoid.configuration.lastSelectedTimezone ? Font.Bold : Font.Normal
                    wrapMode: Text.NoWrap
                    elide: Text.ElideNone

                    Clock {
                        id: clock
                        timeZone: modelData
                        trackSeconds: Plasmoid.configuration.showSeconds
                    }
                }
            }
        }
    }
}
