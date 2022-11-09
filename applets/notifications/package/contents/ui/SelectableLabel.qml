/*
    SPDX-FileCopyrightText: 2011 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2014, 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.8
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

import org.kde.plasma.private.notifications 2.0 as Notifications

PlasmaComponents3.ScrollView {
    id: bodyTextContainer

    property alias text: bodyText.text

    property int cursorShape

    property QtObject contextMenu: null
    property ListView listViewParent: null

    signal clicked(var mouse)
    signal linkActivated(string link)

    leftPadding: mirrored && !Kirigami.Settings.isMobile ? PlasmaComponents3.ScrollBar.vertical.width : 0
    rightPadding: !mirrored && !Kirigami.Settings.isMobile ? PlasmaComponents3.ScrollBar.vertical.width : 0

    // HACK: workaround for https://bugreports.qt.io/browse/QTBUG-83890
    PlasmaComponents3.ScrollBar.horizontal.policy: PlasmaComponents3.ScrollBar.AlwaysOff

    PlasmaComponents3.TextArea {
        id: bodyText
        enabled: !Kirigami.Settings.isMobile
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        background: null
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

            // Pass wheel events to ListView to make scrolling work in FullRepresentation.
            onWheel: {
                if (bodyTextContainer.listViewParent
                    && ((wheel.angleDelta.y > 0 && !bodyTextContainer.listViewParent.atYBeginning)
                        || (wheel.angleDelta.y < 0 && !bodyTextContainer.listViewParent.atYEnd))) {
                    bodyTextContainer.listViewParent.contentY -= wheel.angleDelta.y;
                    wheel.accepted = true;
                } else {
                    wheel.accepted = false;
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
