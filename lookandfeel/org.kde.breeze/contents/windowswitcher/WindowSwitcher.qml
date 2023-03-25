/*
    SPDX-FileCopyrightText: 2011 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kwin 3.0 as KWin

KWin.TabBoxSwitcher {
    id: tabBox

    readonly property real screenFactor: screenGeometry.width / screenGeometry.height

    currentIndex: thumbnailListView.currentIndex

    PlasmaCore.Dialog {
        id: dialog
        location: Qt.application.layoutDirection === Qt.RightToLeft ? PlasmaCore.Types.RightEdge : PlasmaCore.Types.LeftEdge
        visible: tabBox.visible
        flags: Qt.X11BypassWindowManagerHint
        x: screenGeometry.x + (Qt.application.layoutDirection === Qt.RightToLeft ? screenGeometry.width - width : 0)
        y: screenGeometry.y

        mainItem: PlasmaComponents.ScrollView {
            id: dialogMainItem

            focus: true

            contentWidth: tabBox.screenGeometry.width * 0.15
            height: tabBox.screenGeometry.height - dialog.margins.top - dialog.margins.bottom

            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true

            ListView {
                id: thumbnailListView
                focus: true
                model: tabBox.model
                spacing: PlasmaCore.Units.smallSpacing
                highlightMoveDuration: PlasmaCore.Units.longDuration
                highlightResizeDuration: 0

                Connections {
                    target: tabBox
                    function onCurrentIndexChanged() {
                        thumbnailListView.currentIndex = tabBox.currentIndex;
                        thumbnailListView.positionViewAtIndex(thumbnailListView.currentIndex, ListView.Contain)
                    }
                }

                delegate: MouseArea {
                    Accessible.name: model.caption
                    Accessible.role: Accessible.Client

                    width: thumbnailListView.width
                    height: delegateColumn.height + 2 * delegateColumn.y

                    onClicked: {
                        if (tabBox.noModifierGrab) {
                            tabBox.model.activate(index);
                        } else {
                            thumbnailListView.currentIndex = index;
                        }
                    }

                    ColumnLayout {
                        id: delegateColumn
                        anchors.horizontalCenter: parent.horizontalCenter
                        // anchors.centerIn causes layouting glitches
                        y: PlasmaCore.Units.smallSpacing
                        width: parent.width - 2 * PlasmaCore.Units.smallSpacing
                        spacing: PlasmaCore.Units.smallSpacing

                        Item {
                            Layout.fillWidth: true
                            implicitHeight: Math.round(delegateColumn.width / tabBox.screenFactor)

                            KWin.WindowThumbnail {
                                anchors.fill: parent
                                wId: windowId
                            }
                        }

                        RowLayout {
                            spacing: PlasmaCore.Units.smallSpacing
                            Layout.fillWidth: true

                            PlasmaCore.IconItem {
                                Layout.preferredHeight: PlasmaCore.Units.iconSizes.medium
                                Layout.preferredWidth: PlasmaCore.Units.iconSizes.medium
                                source: model.icon
                                usesPlasmaTheme: false
                            }

                            PlasmaExtras.Heading {
                                Layout.fillWidth: true
                                height: undefined
                                level: 4
                                text: model.caption
                                elide: Text.ElideRight
                                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                                maximumLineCount: 2
                                lineHeight: 0.95
                                textFormat: Text.PlainText
                            }
                        }
                    }
                }

                highlight: PlasmaExtras.Highlight {}
            }
        }
    }
}

