/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts
import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore
import org.kde.kirigami as Kirigami

import org.kde.kcmutils // For KCMLauncher
import org.kde.config // KAuthorized
import org.kde.plasma.private.devicenotifier as DN

PlasmoidItem {
    id: devicenotifier

    DN.DeviceFilterControl {
        id: filterModel

        filterType: {
            if (Plasmoid.configuration.allDevices) {
                return DN.DeviceFilterControl.All
            } else if (Plasmoid.configuration.removableDevices) {
                return DN.DeviceFilterControl.Removable
            } else {
                return DN.DeviceFilterControl.Unremovable
            }
        }

        onLastUdiChanged: {
            if (lastDeviceAdded) {
                if (Plasmoid.configuration.popupOnNewDevice) {
                    devicenotifier.expanded = true;
                    fullRepresentationItem.spontaneousOpen = true;
                }
                devicenotifier.popupIcon = "preferences-desktop-notification";
                popupIconTimer.restart();
            }
        }
    }

    readonly property bool openAutomounterKcmAuthorized: KAuthorized.authorizeControlModule("device_automounter_kcm")

    readonly property bool inPanel: (Plasmoid.location === PlasmaCore.Types.TopEdge
        || Plasmoid.location === PlasmaCore.Types.RightEdge
        || Plasmoid.location === PlasmaCore.Types.BottomEdge
        || Plasmoid.location === PlasmaCore.Types.LeftEdge)

    property string popupIcon: ""

    property bool itemClicked: false
    property int currentIndex: -1
    property var connectedRemovables: []
    property int mountedRemovables: 0

    signal unmountAllRequested

    // QTBUG-50380: As soon as the item gets removed from the model, all of ListView's
    // properties (count, contentHeight) pretend the delegate doesn't exist anymore
    // causing our "No devices" heading to overlap with the remaining device
    property bool isMessageHighlightAnimatorRunning: false

    switchWidth: Kirigami.Units.gridUnit * 10
    switchHeight: Kirigami.Units.gridUnit * 10

    toolTipMainText: filterModel.lastUdi !== "" ? i18n("Most Recent Device") : i18n("No Devices Available")
    toolTipSubText: {
        if (filterModel.lastUdi !== "") {
            return filterModel.lastDescription;
        }
        return ""
    }
    Plasmoid.icon: {
        let iconName;
        if (popupIcon !== ""){
            iconName = popupIcon;
        } else if (filterModel.lastUdi !== "") {
            iconName = filterModel.lastIcon;
        } else {
            iconName = "device-notifier";
        }

        // We want to do this here rather than in the model because we don't always
        // want symbolic icons everywhere, but we do know that we always want them
        // for the panel icon
        if (inPanel) {
            return symbolicizeIconName(iconName);
        }

        return iconName;
    }

    Plasmoid.status: (filterModel.deviceCount > 0 || isMessageHighlightAnimatorRunning) ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus

    fullRepresentation: FullRepresentation {
        model: filterModel
    }

    PlasmaCore.Action {
        id: configureAction
        icon.name: "configure"
        text: i18nc("Open auto mounter kcm", "Configure Removable Devices…")
        visible: devicenotifier.openAutomounterKcmAuthorized
        shortcut: "Alt+D, S"
        onTriggered: KCMLauncher.openSystemSettings("kcm_device_automounter")
    }

    Component.onCompleted: {
        if (devicenotifier.deviceCount === 0) {
            Plasmoid.status = PlasmaCore.Types.PassiveStatus;
        }
        Plasmoid.setInternalAction("configure", configureAction);
    }

    PlasmaCore.ActionGroup {
        id: devicesGroup
    }
    Plasmoid.contextualActions: [
        PlasmaCore.Action {
            text: i18n("Remove All")
            visible: filterModel.unmountableCount > 0
            onTriggered: filterModel.unmountAllRemovables()
        },
        PlasmaCore.Action {
            isSeparator: true
        },
        PlasmaCore.Action {
            text: i18n("Removable Devices")
            icon.name: "drive-removable-media"
            checkable: true
            checked: Plasmoid.configuration.removableDevices
            actionGroup: devicesGroup
            onTriggered: checked => {
                if (!checked) {
                    return;
                }
                Plasmoid.configuration.removableDevices = true;
                Plasmoid.configuration.nonRemovableDevices = false;
                Plasmoid.configuration.allDevices = false;
            }
        },
        PlasmaCore.Action {
            text: i18n("Non Removable Devices")
            icon.name: "drive-harddisk"
            checkable: true
            checked: Plasmoid.configuration.nonRemovableDevices
            actionGroup: devicesGroup
            onTriggered: checked => {
                if (!checked) {
                    return;
                }
                Plasmoid.configuration.removableDevices = false;
                Plasmoid.configuration.nonRemovableDevices = true;
                Plasmoid.configuration.allDevices = false;
            }
        },
        PlasmaCore.Action {
            text: i18n("All Devices")
            checkable: true
            checked: Plasmoid.configuration.allDevices
            actionGroup: devicesGroup
            onTriggered: checked => {
                if (!checked) {
                    return;
                }
                Plasmoid.configuration.removableDevices = false;
                Plasmoid.configuration.nonRemovableDevices = false;
                Plasmoid.configuration.allDevices = true;
            }
        },
        PlasmaCore.Action {
            isSeparator: true
        },
        PlasmaCore.Action {
            text: i18n("Show popup when new device is plugged in")
            checkable: true
            checked: Plasmoid.configuration.popupOnNewDevice
            onTriggered: checked => {
                Plasmoid.configuration.popupOnNewDevice = checked;
            }
        },
        PlasmaCore.Action {
            isSeparator: true
        }
    ]

    onExpandedChanged: {
        filterModel.isVisible = devicenotifier.expanded;
        popupEventSlot(devicenotifier.expanded);
    }

    function popupEventSlot(popped) {
        if (!popped) {
            // reset the property that lets us remember if an item was clicked
            // (versus only hovered) for autohide purposes
            devicenotifier.itemClicked = true;
            devicenotifier.currentIndex = -1;
        }
    }

    function symbolicizeIconName(iconName) {
        const symbolicSuffix = "-symbolic";
        if (iconName.endsWith(symbolicSuffix)) {
            return iconName;
        }

        return iconName + symbolicSuffix;
    }


    Timer {
        id: popupIconTimer
        interval: 3000
        onTriggered: devicenotifier.popupIcon  = "";
    }
}
