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
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.11 as Kirigami

import org.kde.plasma.private.notifications 2.0 as Notifications

// NOTE This wrapper item is needed for QQC ScrollView to work
// In NotificationItem we just do SelectableLabel {} and then it gets confused
// as to which is the "contentItem"
PlasmaComponents3.ScrollView {
    id: bodyTextContainer

    property alias text: bodyText.text

    property int cursorShape

    property QtObject contextMenu: null

    signal clicked(var mouse)
    signal linkActivated(string link)

    implicitHeight: Math.min(bodyText.implicitHeight, PlasmaCore.Units.gridUnit * 5)

    PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff
    contentWidth: availableWidth

    PlasmaComponents3.TextArea {
        id: bodyText
        enabled: !Kirigami.Settings.isMobile
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        background: Item {}
        // Work around Qt bug where NativeRendering breaks for non-integer scale factors
        // https://bugreports.qt.io/browse/QTBUG-67007
        renderType: Screen.devicePixelRatio % 1 !== 0 ? Text.QtRendering : Text.NativeRendering
        // Selectable only when we are in desktop mode
        selectByMouse: !Kirigami.Settings.tabletMode

        readOnly: true
        wrapMode: Text.Wrap
        textFormat: TextEdit.RichText

        onLinkActivated: bodyTextContainer.linkActivated(link)

        // Handle left-click
        Notifications.TextEditClickHandler {
            target: bodyText
            onClicked: {
                bodyTextContainer.clicked(null);
            }
        }

        // Handle right-click and cursorShape
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton

            cursorShape: {
                if (bodyText.hoveredLink) {
                    return Qt.PointingHandCursor;
                } else if (bodyText.selectionStart !== bodyText.selectionEnd) {
                    return Qt.IBeamCursor;
                } else {
                    return bodyTextContainer.cursorShape || Qt.IBeamCursor;
                }
            }

            onPressed: {
                contextMenu = contextMenuComponent.createObject(bodyText);
                contextMenu.link = bodyText.linkAt(mouse.x, mouse.y);

                contextMenu.closed.connect(function() {
                    contextMenu.destroy();
                    contextMenu = null;
                });
                contextMenu.open(mouse.x, mouse.y);
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
