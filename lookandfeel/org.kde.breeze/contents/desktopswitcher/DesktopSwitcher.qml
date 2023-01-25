/*
    SPDX-FileCopyrightText: 2012 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.kwin 2.0 as KWin

KWin.Switcher {
    id: tabBox
    currentIndex: listView.currentIndex
    property bool allDesktops: true
    property int textMargin: 2

    PlasmaCore.Dialog {
        id: dialog
        location: PlasmaCore.Types.Floating
        visible: tabBox.visible
        flags: Qt.X11BypassWindowManagerHint
        x: tabBox.screenGeometry.x + tabBox.screenGeometry.width * 0.5 - dialogMainItem.width * 0.5
        y: tabBox.screenGeometry.y + tabBox.screenGeometry.height * 0.5 - dialogMainItem.height * 0.5

        mainItem: Item {
            id: dialogMainItem
            property int optimalWidth: listView.maxRowWidth
            property int optimalHeight: listView.rowHeight * listView.count + clientArea.height + 4
            property bool canStretchX: true
            property bool canStretchY: false
            width: Math.min(Math.max(tabBox.screenGeometry.width * 0.2, optimalWidth), tabBox.screenGeometry.width * 0.8)
            height: Math.min(optimalHeight, tabBox.screenGeometry.height * 0.8)

            // just to get the margin sizes
            PlasmaCore.FrameSvgItem {
                id: hoverItem
                imagePath: "widgets/viewitem"
                prefix: "hover"
                visible: false
            }

            // delegate
            Component {
                id: listDelegate
                Item {
                    id: delegateItem
                    width: listView.width
                    height: listView.rowHeight
                    PlasmaCore.IconItem {
                        id: iconElement
                        source: "user-desktop"
                        width: 32
                        height: 32
                        anchors {
                            topMargin: (listView.rowHeight - 32) / 2
                            left: parent.left
                            top: parent.top
                            leftMargin: hoverItem.margins.left
                        }
                    }
                    Text {
                        id: captionItem
                        horizontalAlignment: Text.AlignHCenter
                        text: display
                        font.bold: true
                        color: PlasmaCore.Theme.textColor
                        elide: Text.ElideMiddle
                        anchors {
                            left: iconElement.right
                            right: parent.right
                            top: parent.top
                            topMargin: tabBox.textMargin + hoverItem.margins.top
                            rightMargin: hoverItem.margins.right
                            leftMargin: tabBox.textMargin
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            listView.currentIndex = index;
                        }
                    }
                }
            }
            ListView {
                id: listView
                function calculateMaxRowWidth() {
                    var width = 0;
                    var textElement = Qt.createQmlObject('import QtQuick 2.0;' + 'Text {\n' + '     text: "' + tabBox.model.longestCaption() + '"\n' + '     font.bold: true\n' + '     visible: false\n' + '}', listView, "calculateMaxRowWidth");
                    width = Math.max(textElement.width, width);
                    textElement.destroy();
                    return width + 32 + hoverItem.margins.right + hoverItem.margins.left;
                }
                /**
                * Calculates the height of one row based on the text height and icon size.
                * @return Row height
                **/
                function calcRowHeight() {
                    var textElement = Qt.createQmlObject('import QtQuick 2.0;' + 'Text {\n' + '     text: "Some Text"\n' + '     font.bold: true\n' + '     visible: false\n' + '}', listView, "calcRowHeight");
                    var height = textElement.height;
                    textElement.destroy();
                    // icon size or two text elements and margins and hoverItem margins
                    return Math.max(32, height + tabBox.textMargin * 2 + hoverItem.margins.top + hoverItem.margins.bottom);
                }
                model: tabBox.model
                // the maximum text width + icon item width (32 + 4 margin) + margins for hover item
                property int maxRowWidth: calculateMaxRowWidth()
                property int rowHeight: calcRowHeight()
                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    bottom: clientArea.top
                    bottomMargin: clientArea.topMargin
                }
                clip: true
                delegate: listDelegate
                highlight: PlasmaCore.FrameSvgItem {
                    id: highlightItem
                    imagePath: "widgets/viewitem"
                    prefix: "hover"
                    width: listView.width
                }
                highlightMoveDuration: PlasmaCore.Units.longDuration
                boundsBehavior: Flickable.StopAtBounds
                Connections {
                    target: tabBox
                    function onCurrentIndexChanged() {
                        listView.currentIndex = tabBox.currentIndex;
                    }
                }
            }
            Component {
                id: clientIconDelegate
                PlasmaCore.IconItem {
                    source: model.icon
                    usesPlasmaTheme: false
                    width: 16
                    height: 16
                }
            }
            Item {
                id: clientArea
                VisualDataModel {
                    id: desktopClientModel
                    property alias desktopIndex: listView.currentIndex
                    model: tabBox.model
                    delegate: clientIconDelegate
                    onDesktopIndexChanged: {
                        desktopClientModel.rootIndex = desktopClientModel.parentModelIndex();
                        desktopClientModel.rootIndex = desktopClientModel.modelIndex(desktopClientModel.desktopIndex);
                    }
                }
                ListView {
                    id: iconsListView
                    model: desktopClientModel
                    clip: true
                    orientation: ListView.Horizontal
                    spacing: 4
                    anchors {
                        fill: parent
                        leftMargin: 34
                    }
                }
                height: 18
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    topMargin: 2
                }
            }
        }
    }
}
