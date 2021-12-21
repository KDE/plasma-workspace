/*
 *  SPDX-FileCopyrightText: 2021 Devin Lin <espidev@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtGraphicalEffects 1.12
import org.kde.kirigami 2.18 as Kirigami

/**
 * Dialog used on desktop. Uses SSDs (as opposed to CSDs).
 */
Item {
    id: root

    property alias mainItem: contentsControl.contentItem
    property alias mainText: titleHeading.text
    property alias subtitle: subtitleLabel.text
    property alias iconName: icon.source
    property list<Kirigami.Action> actions
    readonly property alias dialogButtonBox: footerButtonBox

    property Window window
    implicitHeight: column.implicitHeight
    implicitWidth: column.implicitWidth
    readonly property real minimumHeight: column.Layout.minimumHeight
    readonly property real minimumWidth: column.Layout.minimumWidth
    readonly property int flags: Qt.Dialog
    property alias standardButtons: footerButtonBox.standardButtons

    function present() {
        window.show()
    }

    ColumnLayout {
        id: column
        spacing: 0
        anchors.fill: parent
        
        RowLayout {
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.bottomMargin: Kirigami.Units.gridUnit
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.largeSpacing
            
            spacing: Kirigami.Units.gridUnit
            
            Kirigami.Icon {
                id: icon
                visible: source
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                implicitWidth: Kirigami.Units.iconSizes.large
                implicitHeight: Kirigami.Units.iconSizes.large
            }
            
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Heading {
                    id: titleHeading
                    text: root.title
                    Layout.fillWidth: true
                    level: 2
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                }
                Label {
                    id: subtitleLabel
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    visible: text.length > 0
                }
                Control {
                    id: contentsControl
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    leftPadding: 0
                    rightPadding: 0
                    bottomPadding: 0
                    topPadding: 0
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Item {
                Layout.fillWidth: true
            }
            DialogButtonBox {
                id: footerButtonBox
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                onAccepted: root.window.accept()
                onRejected: root.window.reject()

                Repeater {
                    model: root.actions

                    delegate: Button {
                        action: modelData
                    }
                }
            }
        }
    }
}

