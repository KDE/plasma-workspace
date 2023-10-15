/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>
    SPDX-FileCopyrightText: 2012 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.0
import QtQuick.Controls 2.12 as QQC2
import QtQml.Models 2.14

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.20 as Kirigami

import org.kde.kquickcontrolsaddons 2.0

PlasmaExtras.ExpandableListItem {
    id: deviceItem

    property string udi

    // type: (key: keyof(sdSource.data[]), fallback: V) -> V
    function sdSourceProperty(key: string, fallback: var): var {
        return sdSource.data[udi]?.[key] ?? fallback;
    }

    readonly property int state: sdSourceProperty("State", 0)
    readonly property int operationResult: (model["Operation result"])

    readonly property bool isMounted: devicenotifier.isMounted(udi)

    readonly property bool hasMessage: statusSource.lastUdi == udi && statusSource.data[statusSource.last] ? true : false
    readonly property var message: hasMessage ? statusSource.data[statusSource.last] || ({}) : ({})

    // Types
    function deviceIncludesType(type: string): bool {
        return model["Device Types"]?.includes(type) ?? false;
    }
    function hasStorageAccess(): bool {
        return deviceIncludesType("Storage Access");
    }
    function hasPortableMediaPlayer(): bool {
        return deviceIncludesType("Portable Media Player");
    }
    // Protocols
    function deviceSupportsProtocol(protocol: string): bool {
        return model["Supported Protocols"]?.includes(protocol) ?? false;
    }
    function supportsMTP(): bool {
        return deviceSupportsProtocol("mtp");
    }
    function supportsAFC(): bool {
        return deviceSupportsProtocol("afc");
    }
    // Storage
    function freeSpace(): real {
        return sdSourceProperty("Free Space", -1);
    }
    function totalSpace(): real {
        return sdSourceProperty("Size", -1);
    }
    function freeSpaceKnown(): bool {
        return freeSpace() > 0 && totalSpace() > 0;
    }
    function isRootVolume(): bool {
        return sdSourceProperty("File Path", "") === "/";
    }
    function isRemovable(): bool {
        return sdSourceProperty("Removable", false);
    }

    onOperationResultChanged: {
        if (!popupIconTimer.running) {
            if (operationResult == 1) {
                devicenotifier.popupIcon = "dialog-ok"
                popupIconTimer.restart()
            } else if (operationResult == 2) {
                devicenotifier.popupIcon = "dialog-error"
                popupIconTimer.restart()
            }
        }
    }

    onHasMessageChanged: {
        if (deviceItem.hasMessage) {
            messageHighlight.highlight(this)
        }
    }

    Connections {
        target: unmountAll
        function onClicked() {
            removableActionTriggered();
        }
    }

    Connections {
         target: devicenotifier
         function onUnmountAllRequested() {
             removableActionTriggered();
         }
     }

    // this keeps the delegate around for 5 seconds after the device has been
    // removed in case there was a message, such as "you can now safely remove this"
    ListView.onRemove: removeAnimation.start()

    SequentialAnimation {
        id: removeAnimation
        running: false
        PropertyAction { target: deviceItem; property: "ListView.delayRemove"; value: deviceItem.hasMessage }
        PropertyAction { target: deviceItem; property: "enabled"; value: false }
        // Reset action model to hide the arrow
        PropertyAction { target: deviceItem; property: "contextualActions"; value: [] }
        PropertyAction { target: deviceItem; property: "icon"; value: statusSource.lastIcon }
        PropertyAction { target: deviceItem; property: "title"; value: statusSource.lastDescription }
        PropertyAction { target: deviceItem; property: "subtitle"; value: statusSource.lastMessage }
        PauseAnimation { duration: messageHighlightAnimator.duration }
        // Otherwise the last message will show again when this device reappears
        ScriptAction { script: statusSource.clearMessage(); }
        // Otherwise there are briefly multiple highlight effects
        PropertyAction { target: devicenotifier; property: "currentIndex"; value: -1 }
        PropertyAction { target: deviceItem; property: "ListView.delayRemove"; value: false }
    }

    Timer {
        id: updateStorageSpaceTimer
        interval: 5000
        repeat: true
        running: deviceItem.isMounted && devicenotifier.expanded
        triggeredOnStart: true     // Update the storage space as soon as we open the plasmoid
        onTriggered: {
            const service = sdSource.serviceForSource(udi);
            const operation = service.operationDescription("updateFreespace");
            service.startOperationCall(operation);
        }
    }

    Timer {
        id: unmountTimer
        interval: 1000
        repeat: false
    }

    Component {
        id: deviceActionComponent
        QQC2.Action { }
    }

    function removableActionTriggered() {
        if (isRemovable() && isMounted) {
            actionTriggered();
        }
    }

    function actionTriggered() {
        let service
        let operationName
        let operation
        const wasMounted = isMounted;
        if (!hasStorageAccess() || !isRemovable() || !isMounted) {
            service = hpSource.serviceForSource(udi);
            operation = service.operationDescription('invokeAction');
            operation.predicate = "openWithFileManager.desktop";

            if (!hasStorageAccess() && hasPortableMediaPlayer()) {
                if (deviceItem.supportsMTP()) {
                    operation.predicate = "solid_mtp.desktop" // this lives in kio-extras!
                } else if (deviceItem.supportsAFC()) {
                    operation.predicate = "solid_afc.desktop" // this lives in kio-extras!
                }
            }
        } else {
            service = sdSource.serviceForSource(udi);
            operation = service.operationDescription("unmount");
            unmountTimer.restart();
        }
        service.startOperationCall(operation);
        if (wasMounted) {
            deviceItem.collapse();
        }
    }


    // When there's no better icon available, show a placeholder icon instead
    // of nothing
    icon: sdSource.data[udi] ? sdSource.data[udi].Icon : "device-notifier"

    iconEmblem: {
        if (deviceItem.hasMessage) {
            if (deviceItem.message.solidError === 0) {
                return "emblem-information"
            } else {
                return "emblem-error"
            }
        } else if (deviceItem.state == 0 && Emblems && Emblems[0]) {
            return Emblems[0]
        } else {
            return ""
        }
    }

    title: sdSource.data[udi] ? sdSource.data[udi].Description : ""

    subtitle: {
        if (deviceItem.hasMessage) {
            return deviceItem.message.error
        }
        if (deviceItem.state == 0) {
            if (!hpSource.data[udi]) {
                return ""
            }
            if (freeSpaceKnown()) {
                const freeSpaceText = sdSource.data[udi]["Free Space Text"]
                const totalSpaceText = sdSource.data[udi]["Size Text"]
                return i18nc("@info:status Free disk space", "%1 free of %2", freeSpaceText, totalSpaceText)
            }
            return ""
        } else if (deviceItem.state == 1) {
            return i18nc("Accessing is a less technical word for Mounting; translation should be short and mean \'Currently mounting this device\'", "Accessing…")
        } else if (unmountTimer.running) {
            // Unmounting; shown if unmount takes less than 1 second
            return i18nc("Removing is a less technical word for Unmounting; translation should be short and mean \'Currently unmounting this device\'", "Removing…")
        } else {
            // Unmounting; shown if unmount takes longer than 1 second
            return i18n("Don't unplug yet! Files are still being transferred…")
        }
    }

    subtitleCanWrap: true

    // Color the subtitle red for disks with less than 5% free space
    subtitleColor: {
        if (freeSpaceKnown()) {
            if (freeSpace() / totalSpace() <= 0.05) {
                return Kirigami.Theme.negativeTextColor
            }
        }
        return Kirigami.Theme.textColor
    }

    defaultActionButtonAction: QQC2.Action {
        icon.name: {
            if (deviceItem.isRemovable()) {
                return deviceItem.isMounted ? "media-eject" : "document-open-folder"
            } else {
                return "document-open-folder"
            }
        }
        text: {
            // TODO: this entire logic and the semi-replication thereof in actionTriggered is really silly.
            //  We have a fairly exhaustive predicate system, we should use it to assertain if a given udi is actionable
            //  and then we simply pick the sensible default action of a suitable predicate.
            // - It's possible for there to be no StorageAccess (e.g. MTP devices don't have one)
            // - It's possible for the root volume to be on a removable disk
            if (!deviceItem.hasStorageAccess() || !deviceItem.isRemovable() || deviceItem.isRootVolume()) {
                return i18n("Open in File Manager")
            } else {
                if (!deviceItem.isMounted) {
                    return i18n("Mount and Open")
                } else if (deviceItem.deviceIncludesType("OpticalDisc")) {
                    return i18n("Eject")
                } else {
                    return i18n("Safely remove")
                }
            }
        }
        onTriggered: actionTriggered()
    }

    isBusy: deviceItem.state != 0

    // We need a JS array full of QQC2 actions; this Instantiator creates them
    // from the actions list of the data source
    Instantiator {
        model: hpSource.data[udi] ? hpSource.data[udi].actions : []
        delegate: QQC2.Action {
            text: modelData.text
            icon.name: modelData.icon
            // We only want to show the "Show in file manager" action for
            // mounted removable disks (for everything else, this action is
            // already the primary one shown on the list item)
            enabled: {
                if (modelData.predicate != "openWithFileManager.desktop") {
                    return true;
                }
                return deviceItem.isRemovable() && deviceItem.isMounted;
            }
            onTriggered: {
                const service = hpSource.serviceForSource(udi);
                const operation = service.operationDescription('invokeAction');
                operation.predicate = modelData.predicate;
                service.startOperationCall(operation);
                devicenotifier.currentIndex = -1;
            }
        }
        onObjectAdded: (index, object) => deviceItem.contextualActions.push(object)
        onObjectRemoved: (index, object) => {
            deviceItem.contextualActions = Array.prototype.slice.call(deviceItem.contextualActions)
                .filter(action => action !== object);
        }
    }

    // "Mount" action that does not open it in the file manager
    contextualActions: QQC2.Action {
        text: i18n("Mount")
        icon.name: "media-mount"

        // Only show for unmounted removable devices not in MTP mode
        enabled: deviceItem.isRemovable() && !deviceItem.isMounted && !deviceItem.supportsMTP()

        onTriggered: {
            const service = sdSource.serviceForSource(udi);
            const operation = service.operationDescription("mount");
            service.startOperationCall(operation);
        }
    }
}
