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
     * This property holds the preferred height of the dialog.
     * 
     * The content will receive a hint for how tall it should be to have
     * the dialog to be this height.
     * 
     * If the content, header or footer require more space, then the height
     * of the dialog will expand to the necessary amount of space.
     */
    property real preferredHeight: -1

    /**
     * This property holds the preferred width of the dialog.
     * 
     * The content will receive a hint for how wide it should be to have
     * the dialog be this wide.
     * 
     * If the content, header or footer require more space, then the width
     * of the dialog will expand to the necessary amount of space.
     */
    property real preferredWidth: -1
    
    /**
     * This property holds the QQC2 DialogButtonBox used in the footer of the dialog.
     */
    readonly property alias dialogButtonBox: footerButtonBox
    
    width: column.implicitWidth
    height: column.implicitHeight + footer.implicitHeight
    
    flags: Qt.Dialog

    visible: false
    
    Kirigami.Separator { 
        anchors.left: parent.left 
        anchors.right: parent.right
        anchors.top: parent.top
    }
    
    ColumnLayout {
        id: column
        spacing: 0
        
        RowLayout {
            Layout.maximumWidth: root.maximumWidth
            Layout.maximumHeight: root.maximumHeight - footer.height
            Layout.preferredWidth: root.preferredWidth
            Layout.preferredHeight: root.preferredHeight - footer.height
            
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

