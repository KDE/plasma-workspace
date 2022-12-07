/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQml 2.15

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PC3
import org.kde.plasma.plasmoid 2.0

/**
 * [Album Art][Now Playing]
 */
MouseArea {
    id: compactRepresentation

    Layout.preferredWidth: {
        switch (compactRepresentation.layoutForm) {
        case CompactRepresentation.LayoutType.VerticalPanel:
        case CompactRepresentation.LayoutType.VerticalDesktop:
            return compactRepresentation.parent.width;
        case CompactRepresentation.LayoutType.HorizontalPanel:
        case CompactRepresentation.LayoutType.HorizontalDesktop:
            return iconLoader.active ? iconLoader.item.implicitWidth : playerRow.width;
        case CompactRepresentation.LayoutType.IconOnly:
        default:
            return -1;
        }
    }
    Layout.preferredHeight: {
        switch (compactRepresentation.layoutForm) {
        case CompactRepresentation.LayoutType.VerticalPanel:
            return iconLoader.active ? compactRepresentation.parent.width : playerRow.height;
        default:
            return -1;
        }
    }
    Layout.maximumHeight: Layout.preferredHeight

    enum LayoutType {
        Tray,
        HorizontalPanel,
        VerticalPanel,
        HorizontalDesktop,
        VerticalDesktop,
        IconOnly
    }

    property int layoutForm: CompactRepresentation.LayoutType.IconOnly

    Binding on layoutForm {
        when: playerRow.active
        delayed: true
        restoreMode: Binding.RestoreBindingOrValue
        value: {
            if (compactRepresentation.inTray) {
                return CompactRepresentation.LayoutType.Tray;
            } else if (compactRepresentation.inPanel) {
                return compactRepresentation.isVertical ? CompactRepresentation.LayoutType.VerticalPanel : CompactRepresentation.LayoutType.HorizontalPanel;
            } else if (compactRepresentation.parent.width > compactRepresentation.parent.height + playerRow.item.columnSpacing) {
                return CompactRepresentation.LayoutType.HorizontalDesktop;
            } else if (compactRepresentation.parent.height - compactRepresentation.parent.width >= playerRow.item.labelHeight + playerRow.item.rowSpacing) {
                return CompactRepresentation.LayoutType.VerticalDesktop;
            }
            return CompactRepresentation.LayoutType.IconOnly;
        }
    }

    readonly property bool isVertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property bool inPanel: [PlasmaCore.Types.TopEdge, PlasmaCore.Types.RightEdge, PlasmaCore.Types.BottomEdge, PlasmaCore.Types.LeftEdge].includes(Plasmoid.location)
    readonly property bool inTray: parent.objectName === "org.kde.desktop-CompactApplet"

    acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.BackButton | Qt.ForwardButton
    hoverEnabled: true

    property int wheelDelta: 0

    onWheel: {
        const service = mpris2Source.serviceForSource(mpris2Source.current)
        const operation = service.operationDescription("ChangeVolume")
        wheelDelta += wheel.angleDelta.y || wheel.angleDelta.x
        while (wheelDelta >= 120) {
            wheelDelta -= 120;
            operation.delta = volumePercentStep / 100;
            operation.showOSD = true;
            service.startOperationCall(operation);
        }
        while (wheelDelta <= -120) {
            wheelDelta += 120;
            operation.delta = -volumePercentStep / 100;
            operation.showOSD = true;
            service.startOperationCall(operation);
        }
    }

    onClicked: {
        switch (mouse.button) {
        case Qt.MiddleButton:
            root.togglePlaying()
            break
        case Qt.BackButton:
            root.action_previous()
            break
        case Qt.ForwardButton:
            root.action_next()
            break
        default:
            Plasmoid.expanded = !Plasmoid.expanded
        }
    }

    Loader {
        id: iconLoader
        anchors.fill: parent
        visible: active

        active: inTray || !root.track
        sourceComponent: PlasmaCore.IconItem {
            active: compactRepresentation.containsMouse
            source: {
                if (root.isPlaying) {
                    return "media-playback-playing";
                } else if (root.state === "paused") {
                    return "media-playback-paused";
                } else {
                    return "media-playback-stopped";
                }
            }
        }
    }

    Loader {
        id: playerRow

        width: {
            if (!active) {
                return 0;
            }
            switch (compactRepresentation.layoutForm) {
            case CompactRepresentation.LayoutType.VerticalPanel:
            case CompactRepresentation.LayoutType.VerticalDesktop:
                return compactRepresentation.parent.width;
            case CompactRepresentation.LayoutType.HorizontalPanel:
                return item.implicitWidth;
            case CompactRepresentation.LayoutType.HorizontalDesktop:
                return Math.min(item.implicitWidth, compactRepresentation.parent.width);
            case CompactRepresentation.LayoutType.IconOnly:
            default:
                return Math.min(compactRepresentation.parent.width, compactRepresentation.parent.height);
            }
        }

        height: {
            if (!active) {
                return 0;
            }
            switch (compactRepresentation.layoutForm) {
            case CompactRepresentation.LayoutType.VerticalPanel:
                return item.implicitHeight;
            case CompactRepresentation.LayoutType.VerticalDesktop:
            case CompactRepresentation.LayoutType.HorizontalPanel:
            case CompactRepresentation.LayoutType.HorizontalDesktop:
                return compactRepresentation.parent.height;
            case CompactRepresentation.LayoutType.IconOnly:
            default:
                return Math.min(compactRepresentation.parent.width, compactRepresentation.parent.height);
            }
        }

        visible: active

        active: !iconLoader.active
        sourceComponent: GridLayout {
            id: grid
            readonly property real labelHeight: songTitle.contentHeight

            rowSpacing: PlasmaCore.Units.smallSpacing
            columnSpacing: rowSpacing
            flow: {
                switch (compactRepresentation.layoutForm) {
                case CompactRepresentation.LayoutType.VerticalPanel:
                case CompactRepresentation.LayoutType.VerticalDesktop:
                    return GridLayout.TopToBottom;
                default:
                    return GridLayout.LeftToRight;
                }
            }

            Item {
                id: spacerItem
                visible: compactRepresentation.layoutForm === CompactRepresentation.LayoutType.VerticalDesktop
                Layout.fillHeight: true
            }

            AlbumArtStackView {
                id: albumArt

                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: Math.min(compactRepresentation.parent.width, compactRepresentation.parent.height)
                Layout.preferredHeight: Layout.preferredWidth

                inCompactRepresentation: true

                Connections {
                    target: root

                    function onAlbumArtChanged() {
                        albumArt.loadAlbumArt();
                    }
                }

                Component.onCompleted: albumArt.loadAlbumArt()
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignVCenter
                visible: (compactRepresentation.layoutForm !== CompactRepresentation.LayoutType.VerticalPanel
                    && compactRepresentation.layoutForm !== CompactRepresentation.LayoutType.IconOnly)
                    || (compactRepresentation.layoutForm === CompactRepresentation.LayoutType.VerticalPanel
                    && compactRepresentation.parent.width >= PlasmaCore.Units.gridUnit * 5)

                spacing: 0

                // Song Title
                PC3.Label {
                    id: songTitle

                    Layout.fillWidth: true
                    Layout.maximumWidth: compactRepresentation.layoutForm === CompactRepresentation.LayoutType.HorizontalPanel ? PlasmaCore.Units.gridUnit * 10 : -1

                    elide: Text.ElideRight
                    horizontalAlignment: grid.flow === GridLayout.TopToBottom ? Text.AlignHCenter : Text.AlignJustify
                    maximumLineCount: 1
                    text: root.track
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                // Song Artist
                PC3.Label {
                    id: songArtist

                    Layout.fillWidth: true
                    Layout.maximumWidth: songTitle.Layout.maximumWidth
                    visible: root.artist && playerRow.height >= songTitle.contentHeight + contentHeight * 0.8 /* For CJK */ + (compactRepresentation.layoutForm === CompactRepresentation.LayoutType.VerticalDesktop ? albumArt.Layout.preferredHeight + grid.rowSpacing : 0)

                    elide: Text.ElideRight
                    font.pointSize: PlasmaCore.Theme.smallestFont.pointSize
                    horizontalAlignment: songTitle.horizontalAlignment
                    maximumLineCount: 1
                    opacity: 0.6
                    text: root.artist
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }

            Item {
                visible: spacerItem.visible
                Layout.fillHeight: true
            }
        }
    }
}
