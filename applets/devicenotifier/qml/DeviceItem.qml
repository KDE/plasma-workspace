/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQml.Models

import org.kde.plasma.plasmoid
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

import org.kde.kquickcontrolsaddons

PlasmaExtras.ExpandableListItem {
    id: deviceItem

    required property int index

    required property string deviceDescription
    required property string deviceIcon
    required property double deviceFreeSpace
    required property double deviceSize
    required property string deviceFreeSpaceText
    required property string deviceSizeText
    required property bool deviceIsBusy
    required property bool deviceMounted
    required property int deviceState
    required property int deviceOperationResult
    required property string deviceMessage
    required property var deviceActions
    required property var deviceEmblems

    property bool hasMessage: deviceItem.deviceMessage !== ""

    //Shows true whenever the device is idle and present in the system.
    readonly property bool isFree: !deviceItem.deviceIsBusy && deviceItem.deviceState !== DevicesStateMonitor.NotPresent

    //Shows true whenever the device finished some job
    readonly property bool isOperationFinished: deviceItem.isFree && deviceItem.deviceState !== DevicesStateMonitor.Idle

    onIsOperationFinishedChanged: {
        if (deviceItem.isOperationFinished) {
            if (deviceItem.deviceOperationResult === 0) {
                devicenotifier.popupIcon = "dialog-ok"
                popupIconTimer.restart()
            } else if (deviceItem.deviceOperationResult !== 0) {
                devicenotifier.popupIcon = "dialog-error"
                popupIconTimer.restart()
            }
        }
    }

    onDeviceStateChanged: {
        if (deviceItem.deviceState === DevicesStateMonitor.Unmounting) {
            unmountTimer.restart();
        }
    }

    onHasMessageChanged: {
        if (deviceItem.hasMessage) {
            messageHighlight.highlight(this)
        }
    }

    Timer {
        id: unmountTimer
        interval: 1000
        repeat: false
    }

    //show busy indicator whenever is busy (mounting, unmounting or other operations on the device)
    isBusy: deviceItem.deviceIsBusy

    icon: deviceItem.deviceIcon

    iconEmblem: {
        if (deviceItem.isOperationFinished && deviceItem.hasMessage) {
            if (deviceItem.deviceOperationResult === 0) {
                return "emblem-information"
            } else {
                return "emblem-error"
            }
        //if device is not busy then show its emblem
        } else if (!deviceItem.deviceIsBusy && deviceItem.deviceEmblems[0]) {
            return deviceItem.deviceEmblems[0]
        } else {
            return ""
        }
    }

    title: deviceItem.deviceDescription

    subtitle: {
        if (deviceItem.isOperationFinished && deviceItem.hasMessage) {
            return deviceItem.deviceMessage
        }
        if (deviceItem.deviceState === DevicesStateMonitor.Checking) {
            return i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Checking…")
        } else if (deviceItem.deviceState === DevicesStateMonitor.Repairing) {
            return i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Repairing…")
        } else if (!deviceItem.deviceIsBusy) {
            if (deviceItem.deviceFreeSpace > 0 && deviceItem.deviceSize > 0) {
                return i18nc("@info:status Free disk space", "%1 free of %2", deviceItem.deviceFreeSpaceText, deviceItem.deviceSizeText)
            }
            return ""
        } else if (deviceItem.deviceState === DevicesStateMonitor.Mounting) {
            return i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Accessing…")
        } else if (deviceItem.deviceState === DevicesStateMonitor.Unmounting && unmountTimer.running) {
            // Unmounting; shown if unmount takes less than 1 second
            return i18nc("Removing is a less technical word for Unmounting; translation should be short and mean \'Currently unmounting this device\'", "Removing…")
        } else if (deviceItem.deviceState === DevicesStateMonitor.Unmounting) {
            // Unmounting; shown if unmount takes longer than 1 second
            return i18n("Don't unplug yet! Files are still being transferred…")
        }
        return ""
    }

    subtitleCanWrap: true

    // Color the subtitle red for disks with less than 5% free space
    subtitleColor: {
        if (deviceItem.deviceFreeSpace > 0 && deviceItem.deviceSize > 0) {
            if (deviceItem.deviceFreeSpace / deviceItem.deviceSize <= 0.05) {
                return Kirigami.Theme.negativeTextColor
            }
        }
        return Kirigami.Theme.textColor
    }

    defaultActionButtonAction: deviceActions !== undefined && isFree ? defaultAction : null

    QQC2.Action {
        id: defaultAction
        icon.name: deviceActions?.defaultActionIcon ?? ""
        text: deviceActions?.defaultActionText ?? ""
        onTriggered: {
            if (deviceItem.deviceMounted) {
                unmountTimer.restart();
            }
            deviceActions.actionTriggered(deviceActions.defaultActionName)
        }
    }

    customExpandedViewContent: deviceActions !== undefined && deviceActions.rowCount() !== 0 && isFree ? actionComponent : null

    Component {
        id: actionComponent

        ListView {
            id: actionRepeater

            contentWidth: parent.width
            contentHeight: contentItem.height
            width: contentWidth
            height: contentHeight

            model: deviceItem.deviceActions

            delegate: PlasmaComponents3.ToolButton
            {
                width: parent.width
                text: model.Text
                icon.name: model.Icon

                KeyNavigation.up: currentIndex > 0 ? actionRepeater.itemAtIndex(currentIndex - 1) : deviceItem
                KeyNavigation.down: {
                    if (currentIndex <= actionRepeater.count - 1) {
                        return actionRepeater.itemAtIndex(currentIndex + 1);
                    } else {
                        return actionRepeater.itemAtIndex(0);
                    }
                }

                onClicked: {
                    deviceActions.actionTriggered(model.Name)
                }

            }
        }

    }
}
