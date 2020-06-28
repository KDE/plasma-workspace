/*
 * Copyright 2011 Marco Martin <notmart@gmail.com>
 * Copyright 2014, 2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.8
import QtQuick.Window 2.2
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.11 as Kirigami

// NOTE This wrapper item is needed for QQC ScrollView to work
// In NotificationItem we just do SelectableLabel {} and then it gets confused
// as to which is the "contentItem"
Item {
    id: bodyTextContainer

    property alias text: bodyText.text
    property alias font: bodyText.font

    property int cursorShape

    property QtObject contextMenu: null

    signal clicked(var mouse)
    signal linkActivated(string link)

    implicitWidth: bodyText.paintedWidth
    implicitHeight: bodyText.paintedHeight


    PlasmaExtras.ScrollArea {
        id: bodyTextScrollArea

        anchors.fill: parent

        flickableItem.boundsBehavior: Flickable.StopAtBounds
        flickableItem.flickableDirection: Flickable.VerticalFlick
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff

        TextEdit {
            id: bodyText
            width: bodyTextScrollArea.width
            // TODO check that this doesn't causes infinite loops when it starts adding and removing the scrollbar
            //width: bodyTextScrollArea.viewport.width
            enabled: !Kirigami.Settings.isMobile

            color: PlasmaCore.ColorScope.textColor
            selectedTextColor: theme.viewBackgroundColor
            selectionColor: theme.viewFocusColor
            font.capitalization: theme.defaultFont.capitalization
            font.family: theme.defaultFont.family
            font.italic: theme.defaultFont.italic
            font.letterSpacing: theme.defaultFont.letterSpacing
            font.pointSize: theme.defaultFont.pointSize
            font.strikeout: theme.defaultFont.strikeout
            font.underline: theme.defaultFont.underline
            font.weight: theme.defaultFont.weight
            font.wordSpacing: theme.defaultFont.wordSpacing
            // Work around Qt bug where NativeRendering breaks for non-integer scale factors
            // https://bugreports.qt.io/browse/QTBUG-67007
            renderType: Screen.devicePixelRatio % 1 !== 0 ? Text.QtRendering : Text.NativeRendering
            // Selectable only when we are in desktop mode
            selectByMouse: !Kirigami.Settings.tabletMode
            
            readOnly: true
            wrapMode: Text.Wrap
            textFormat: TextEdit.RichText

            onLinkActivated: bodyTextContainer.linkActivated(link)

            // ensure selecting text scrolls the view as needed...
            onCursorRectangleChanged: {
                var flick = bodyTextScrollArea.flickableItem
                if (flick.contentY >= cursorRectangle.y) {
                    flick.contentY = cursorRectangle.y
                } else if (flick.contentY + flick.height <= cursorRectangle.y + cursorRectangle.height) {
                    flick.contentY = cursorRectangle.y + cursorRectangle.height - flick.height
                }
            }
            MouseArea {
                property int selectionStart
                property point mouseDownPos: Qt.point(-999, -999);

                anchors.fill: parent
                acceptedButtons: Qt.RightButton | Qt.LeftButton
                cursorShape: {
                    if (bodyText.hoveredLink) {
                        return Qt.PointingHandCursor;
                    } else if (bodyText.selectionStart !== bodyText.selectionEnd) {
                        return Qt.IBeamCursor;
                    } else {
                        return bodyTextContainer.cursorShape || Qt.IBeamCursor;
                    }
                }
                preventStealing: true // don't let us accidentally drag the Flickable

                onPressed: {
                    if (mouse.button === Qt.RightButton) {
                        contextMenu = contextMenuComponent.createObject(bodyText);
                        contextMenu.link = bodyText.linkAt(mouse.x, mouse.y);

                        contextMenu.closed.connect(function() {
                            contextMenu.destroy();
                            contextMenu = null;
                        });
                        contextMenu.open(mouse.x, mouse.y);
                        return;
                    }

                    mouseDownPos = Qt.point(mouse.x, mouse.y);
                    selectionStart = bodyText.positionAt(mouse.x, mouse.y);
                    var pos = bodyText.positionAt(mouse.x, mouse.y);
                    // deselect() would scroll to the end which we don't want
                    bodyText.select(pos, pos);
                }

                onReleased: {
                    // emulate "onClicked"
                    var manhattanLength = Math.abs(mouseDownPos.x - mouse.x) + Math.abs(mouseDownPos.y - mouse.y);
                    if (manhattanLength <= Qt.styleHints.startDragDistance) {
                        var link = bodyText.linkAt(mouse.x, mouse.y);
                        if (link) {
                            Qt.openUrlExternally(link);
                        } else {
                            bodyTextContainer.clicked(mouse);
                        }
                    }

                    // emulate selection clipboard
                    if (bodyText.selectedText) {
                        plasmoid.nativeInterface.setSelectionClipboardText(bodyText.selectedText);
                    }

                    mouseDownPos = Qt.point(-999, -999);
                }

                // HACK to be able to select text whilst still getting all mouse events to the MouseArea
                onPositionChanged: {
                    if (pressed) {
                        var pos = bodyText.positionAt(mouseX, mouseY);
                        if (selectionStart < pos) {
                            bodyText.select(selectionStart, pos);
                        } else {
                            bodyText.select(pos, selectionStart);
                        }
                    }
                }
            }
        }
    }

    Component {
        id: contextMenuComponent

        EditContextMenu {
            target: bodyText
        }
    }
}
