/********************************************************************
 KWin - the KDE window manager
 This file is part of the KDE project.

Copyright (C) 2011 Martin Gräßlin <mgraesslin@kde.org>
Copyright (C) 2013 Marco Martin <mart@kde.org>
Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kwin 2.0 as KWin

KWin.Switcher {
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

        mainItem: PlasmaExtras.ScrollArea {
            id: dialogMainItem

            focus: true

            width: tabBox.screenGeometry.width * 0.15 + (__verticalScrollBar.visible ? __verticalScrollBar.width : 0)
            height: tabBox.screenGeometry.height - dialog.margins.top - dialog.margins.bottom

            LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
            LayoutMirroring.childrenInherit: true

            ListView {
                id: thumbnailListView
                model: tabBox.model
                spacing: units.smallSpacing
                highlightMoveDuration: 250
                highlightResizeDuration: 0

                Connections {
                    target: tabBox
                    onCurrentIndexChanged: {
                        thumbnailListView.currentIndex = tabBox.currentIndex;
                        thumbnailListView.positionViewAtIndex(thumbnailListView.currentIndex, ListView.Contain)
                    }
                }

                delegate: MouseArea {
                    width: thumbnailListView.width
                    height: delegateColumn.implicitHeight + 2 * delegateColumn.y

                    onClicked: thumbnailListView.currentIndex = index

                    ColumnLayout {
                        id: delegateColumn
                        anchors.horizontalCenter: parent.horizontalCenter
                        // anchors.centerIn causes layouting glitches
                        y: units.smallSpacing
                        width: parent.width - 2 * units.smallSpacing
                        spacing: units.smallSpacing

                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.round(width / tabBox.screenFactor)

                            KWin.ThumbnailItem {
                                anchors.fill: parent
                                wId: windowId
                            }
                        }

                        RowLayout {
                            spacing: units.smallSpacing
                            Layout.fillWidth: true

                            PlasmaCore.IconItem {
                                Layout.preferredHeight: units.iconSizes.medium
                                Layout.preferredWidth: units.iconSizes.medium
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
                            }
                        }
                    }
                }

                highlight: PlasmaComponents.Highlight {}
            }
        }
    }
}

