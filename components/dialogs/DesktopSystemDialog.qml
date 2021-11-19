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
import "private" as Private

/**
 * Dialog used on desktop. Uses SSDs (as opposed to CSDs).
 */
Kirigami.AbstractApplicationWindow {
    id: root

    default property alias mainItem: contentsControl.contentItem

    /**
     * Main text of the dialog.
     */
    property alias mainText: titleHeading.text

    /**
     * Subtitle of the dialog.
     */
    property alias subtitle: subtitleLabel.text

    /**
     * This property holds the icon used in the dialog.
     */
    property alias iconName: icon.source

    /**
     * This property holds the list of actions for this dialog.
     *
     * Each action will be rendered as a button that the user will be able
     * to click.
     */
    property list<Kirigami.Action> actions
    
    /**
     * This property holds the QQC2 DialogButtonBox used in the footer of the dialog.
     */
    readonly property alias dialogButtonBox: footerButtonBox

    /**
     * Controls whether the accept button is enabled
     */
    property bool acceptable: true
    
    width: column.implicitWidth
    height: column.implicitHeight + footer.implicitHeight
    minimumHeight: column.Layout.minimumHeight + footer.implicitHeight
    minimumWidth: column.Layout.minimumWidth
    
    flags: Qt.Dialog

    visible: false

    signal accept()
    signal reject()
    property bool accepted: false
    onAccept: accepted = true

    onVisibleChanged: if (!visible && !accepted) {
        root.reject()
    }

    ColumnLayout {
        Keys.onEscapePressed: root.reject()

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
    }
    
    footer: Control {
        leftPadding: Kirigami.Units.largeSpacing
        rightPadding: Kirigami.Units.largeSpacing
        topPadding: Kirigami.Units.largeSpacing
        bottomPadding: Kirigami.Units.largeSpacing
        
        contentItem: RowLayout {
            Item { Layout.fillWidth: true }
            DialogButtonBox {
                id: footerButtonBox
                spacing: Kirigami.Units.smallSpacing
                onAccepted: root.accept()
                onRejected: root.reject()

                Binding {
                    target: footerButtonBox.standardButton(DialogButtonBox.Ok)
                    property: "enabled"
                    when: footerButtonBox.standardButton(DialogButtonBox.Ok)
                    value: root.acceptable
                }

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

