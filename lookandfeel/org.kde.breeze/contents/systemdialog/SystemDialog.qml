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
Item {
    id: root

    property alias mainItem: contentsItem.contentItem
    property alias mainText: titleHeading.text
    property alias subtitle: subtitleLabel.text
    property alias iconName: icon.source
    property list<T.Action> actions
    readonly property alias dialogButtonBox: footerButtonBox

    property Window window
    implicitHeight: column.implicitHeight
    implicitWidth: column.implicitWidth
    readonly property real minimumHeight: column.Layout.minimumHeight + (mainItem ? mainItem.implicitHeight : 0) + footerButtonBox.implicitHeight + root.spacing * 2
    readonly property real minimumWidth: Math.min(Math.round(Screen.width / 3), column.Layout.minimumWidth + (mainItem ? mainItem.implicitWidth : 0) + footerButtonBox.implicitWidth) + root.spacing * 2
    readonly property int flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
    property alias standardButtons: footerButtonBox.standardButtons
    readonly property int spacing: Kirigami.Units.largeSpacing // standard KDE dialog margins

    function present() {
        window.show()
    }

    Kirigami.Separator {
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: root.spacing

        spacing: root.spacing

        // Header area
        RowLayout {
            spacing: root.spacing

            Kirigami.Icon {
                id: icon
                visible: valid
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
        Kirigami.Padding {
            id: contentsItem

            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // Footer area with buttons
        QQC2.DialogButtonBox {
            id: footerButtonBox

            visible: count > 0

            Layout.fillWidth: true

            padding: 0
            topPadding: undefined
            leftPadding: undefined
            rightPadding: undefined
            bottomPadding: undefined
            verticalPadding: undefined
            horizontalPadding: undefined

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
}
