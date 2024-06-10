/*
 *  SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
 *  SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami

/**
 * Dialog used on desktop. Uses SSDs (as opposed to CSDs).
 */
ColumnLayout {
    id: root

    property alias mainItem: contentsControl.contentItem
    property alias mainText: titleHeading.text
    property alias subtitle: subtitleLabel.text
    property alias iconName: icon.source
    property list<T.Action> actions
    readonly property alias dialogButtonBox: footerButtonBox

    property Window window
    readonly property int flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
    property alias standardButtons: footerButtonBox.standardButtons

    readonly property real minimumHeight: implicitHeight
    readonly property real minimumWidth: Math.min(Math.round(Screen.width / 3), implicitWidth)

    function present() {
        window.show();
    }

    spacing: Kirigami.Units.smallSpacing

    Kirigami.Separator {
        Layout.fillWidth: true
    }

    RowLayout {

        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing

        Layout.fillWidth: true
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Icon {
            id: icon
            visible: source
            implicitWidth: Kirigami.Units.iconSizes.large
            implicitHeight: Kirigami.Units.iconSizes.large
        }

        ColumnLayout {
            Layout.fillWidth: true

            spacing: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                id: titleHeading
                Layout.fillWidth: true
                level: 2
                wrapMode: Text.Wrap
                textFormat: Text.RichText
                elide: Text.ElideRight
            }

            QQC2.Label {
                id: subtitleLabel
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                elide: Text.ElideRight
                textFormat: Text.RichText
                visible: text.length > 0
            }
        }
    }

    // Main content area, to be provided by the implementation
    QQC2.Control {
        id: contentsControl

        Layout.fillWidth: true
        Layout.fillHeight: true

        // make sure we don't add padding if there is no content
        visible: contentItem !== null
    }

    // Footer area with buttons
    QQC2.DialogButtonBox {
        id: footerButtonBox

        Layout.fillWidth: true

        visible: count > 0

        onAccepted: root.window.accept()
        onRejected: root.window.reject()

        Repeater {
            model: root.actions

            delegate: QQC2.Button {
                action: modelData
            }
        }
    }
}
