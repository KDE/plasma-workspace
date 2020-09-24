/*
 * Copyright 2019 Kai Uwe Broulik <kde@broulik.de>
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
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents3

RowLayout {
    id: replyRow

    signal beginReplyRequested
    signal replied(string text)

    property bool replying: false

    property alias text: replyTextField.text
    property string placeholderText
    property string buttonIconName
    property string buttonText

    spacing: PlasmaCore.Units.smallSpacing

    function activate() {
        replyTextField.forceActiveFocus();
    }

    PlasmaComponents3.TextField {
        id: replyTextField
        Layout.fillWidth: true
        placeholderText: replyRow.placeholderText
                         || i18ndc("plasma_applet_org.kde.plasma.notifications", "Text field placeholder", "Type a reply...")
        onAccepted: {
            if (replyButton.enabled) {
                replyRow.replied(text);
            }
        }

        // Catches mouse click when reply field is already shown to start a reply
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.IBeamCursor
            visible: !replyRow.replying
            onPressed: replyRow.beginReplyRequested()
        }
    }

    PlasmaComponents3.Button {
        id: replyButton
        icon.name: replyRow.buttonIconName || "document-send"
        text: replyRow.buttonText
              || i18ndc("plasma_applet_org.kde.plasma.notifications", "@action:button", "Send")
        enabled: replyTextField.length > 0
        onClicked: replyRow.replied(replyTextField.text)
    }
}
