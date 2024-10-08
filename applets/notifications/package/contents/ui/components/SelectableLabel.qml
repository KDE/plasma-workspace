/*
    SPDX-FileCopyrightText: 2011 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2014, 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.plasma.components as PlasmaComponents3
import org.kde.kirigami as Kirigami

// TODO: Port to Kirigami.SelectableText/selectableLabel (colors should be fine)
PlasmaComponents3.TextArea {
    id: bodyTextContainer
    property ModelInterface modelInterface

    property int cursorShape
    property QtObject contextMenu: null

    signal clicked(var mouse)

    enabled: !Kirigami.Settings.isMobile
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    background: null
    color: Kirigami.Theme.textColor

    // Selectable only when we are in desktop mode
    selectByMouse: !Kirigami.Settings.tabletMode

    readOnly: true
    wrapMode: TextEdit.Wrap
    textFormat: TextEdit.RichText

    TapHandler {
        acceptedButtons: Qt.LeftButton
        onTapped: bodyTextContainer.clicked(null)
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        cursorShape: {
            if (bodyTextContainer.hoveredLink) {
                return Qt.PointingHandCursor;
            } else if (bodyTextContainer.selectionStart !== bodyTextContainer.selectionEnd) {
                return Qt.IBeamCursor;
            } else {
                return bodyTextContainer.cursorShape || Qt.IBeamCursor;
            }
        }
        onTapped: eventPoint => {
            contextMenu = contextMenuComponent.createObject(bodyTextContainer);
            contextMenu.link = bodyTextContainer.linkAt(eventPoint.position.x, eventPoint.position.y);

            contextMenu.closed.connect(function() {
                contextMenu.destroy();
                contextMenu = null;
            });
            contextMenu.open(eventPoint.position.x, eventPoint.position.y);
        }
    }

    Component {
        id: contextMenuComponent

        EditContextMenu {
            target: bodyTextContainer
        }
    }
}
