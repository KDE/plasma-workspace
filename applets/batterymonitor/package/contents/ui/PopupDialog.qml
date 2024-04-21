/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2013-2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents3
import org.kde.plasma.extras as PlasmaExtras
import org.kde.kirigami as Kirigami

PlasmaExtras.Representation {
    id: dialog

    property alias model: batteryRepeater.model
    property bool pluggedIn
    property int chargeStopThreshold
    property bool isManuallyInhibited
    property bool isManuallyInhibitedError

    property int remainingTime

    property bool profilesInstalled
    property string activeProfile
    property string activeProfileError
    property var profiles

    // List of active power management inhibitions (applications that are
    // blocking sleep and screen locking).
    //
    // type: [{
    //  Icon: string,
    //  Name: string,
    //  Reason: string,
    // }]
    property var inhibitions: []
    property bool inhibitsLidAction

    property string inhibitionReason
    property string degradationReason
    // type: [{ Name: string, Icon: string, Profile: string, Reason: string }]
    required property var profileHolds

    signal inhibitionChangeRequested(bool inhibit)
    signal activateProfileRequested(string profile)

    collapseMarginsHint: true

    KeyNavigation.down: batteryRepeater.headerItem

    contentItem: PlasmaComponents3.ScrollView {
        id: scrollView

        focus: false

        function positionViewAtItem(item) {
            if (!PlasmaComponents3.ScrollBar.vertical.visible) {
                return;
            }
            const rect = powerItemList.mapFromItem(item, 0, 0, item.width, item.height);
            if (rect.y < scrollView.contentItem.contentY) {
                scrollView.contentItem.contentY = rect.y;
            } else if (rect.y + rect.height > scrollView.contentItem.contentY + scrollView.height) {
                scrollView.contentItem.contentY = rect.y + rect.height - scrollView.height;
            }
        }

        ListView {

            id: batteryRepeater

            spacing: Kirigami.Units.smallSpacing * 2


            header: Column {
                    PowerProfileItem {
                    id: powerProfileItem

                    width: scrollView.availableWidth

                    KeyNavigation.up: batteryRepeater.footerItem
                    KeyNavigation.down: batteryRepeater.count > 0 ? batteryRepeater.itemAtIndex(0) : batteryRepeater.footerItem
                    KeyNavigation.backtab: KeyNavigation.up
                    KeyNavigation.tab: KeyNavigation.down

                    profilesInstalled: dialog.profilesInstalled
                    profilesAvailable: dialog.profiles.length > 0
                    activeProfile: dialog.activeProfile
                    activeProfileError: dialog.activeProfileError
                    inhibitionReason: dialog.inhibitionReason
                    degradationReason: dialog.degradationReason
                    profileHolds: dialog.profileHolds

                    onActivateProfileRequested: profile => {
                        dialog.activateProfileRequested(profile);
                    }

                    onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)
                }
            }

            delegate: BatteryItem {
                width: scrollView.availableWidth

                batteryPercent: Percent
                batteryCapacity: Capacity
                batteryEnergy: Energy
                batteryPluggedIn: PluggedIn
                batteryIsPowerSupply: IsPowerSupply
                batteryChargeState: ChargeState
                batteryPrettyName: PrettyName
                batteryType: Type
                remainingTime: dialog.remainingTime


                KeyNavigation.up: index === 0 ? (batteryRepeater.headerItem.visible ? batteryRepeater.headerItem : batteryRepeater.headerItem.KeyNavigation.up) : batteryRepeater.itemAtIndex(index - 1)
                KeyNavigation.down: index + 1 < batteryRepeater.count ? batteryRepeater.itemAtIndex(index + 1) : batteryRepeater.footerItem

                pluggedIn: dialog.pluggedIn
                chargeStopThreshold: dialog.chargeStopThreshold

                KeyNavigation.backtab: KeyNavigation.up
                KeyNavigation.tab: KeyNavigation.down

                Keys.onTabPressed: event => {
                    if (index === batteryRepeater.count - 1) {
                        // Workaround to leave applet's focus on desktop
                        nextItemInFocusChain(false).forceActiveFocus(Qt.TabFocusReason);
                    } else {
                        event.accepted = false;
                    }
                }

                onActiveFocusChanged: if (activeFocus) scrollView.positionViewAtItem(this)
            }

            footer:  PowerManagementItem {
                id: powerManagementItem

                width: scrollView.availableWidth

                KeyNavigation.up: batteryRepeater.itemAtIndex(batteryRepeater.count - 1)
                KeyNavigation.down: null
                KeyNavigation.backtab:KeyNavigation.up
                KeyNavigation.tab: powerManagementItem.manualInhibitionSwitch

                inhibitions: dialog.inhibitions
                isManuallyInhibited: dialog.isManuallyInhibited
                isManuallyInhibitedError: dialog.isManuallyInhibitedError
                inhibitsLidAction: dialog.inhibitsLidAction
                pluggedIn: dialog.pluggedIn

                onInhibitionChangeRequested: inhibit => {
                    batterymonitor.inhibitionChangeRequested(inhibit);
                }
            }
        }
    }
}

