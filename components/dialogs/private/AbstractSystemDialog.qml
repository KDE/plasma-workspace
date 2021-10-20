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
 * Component to create CSD dialogs that come from the system.
 */
Kirigami.AbstractApplicationWindow {
    id: root
    visible: false
    
    flags: Qt.FramelessWindowHint | Qt.Dialog
    Kirigami.Theme.colorSet: Kirigami.Theme.View
    Kirigami.Theme.inherit: false
    color: darkenBackground && visible && visibility === 5 ? Qt.rgba(0, 0, 0, 0.5) : "transparent" // darken when fullscreen
    
    x: root.transientParent ? root.transientParent.x + root.transientParent.width / 2 - root.width / 2 : 0
    y: root.transientParent ? root.transientParent.y + root.transientParent.height / 2 - root.height / 2 : 0
    modality: darkenBackground && visibility !== 5 ? Qt.WindowModal : Qt.NonModal // darken parent window when not fullscreen
    
    Behavior on color {
        ColorAnimation {
            duration: 500
            easing.type: Easing.InOutQuad
        } 
    }
    
    /**
     * This property holds the dialog's contents.
     * 
     * The initial height and width of the dialog is calculated from the 
     * `implicitWidth` and `implicitHeight` of this item.
     */
    default property Item bodyItem
    
    /**
     * This property holds the absolute maximum height the dialog can be
     * (including the header and footer).
     * 
     * The height restriction is solely applied on the content, so if the
     * maximum height given is not larger than the height of the header and
     * footer, it will be ignored.
     * 
     * This is the window height, subtracted by largeSpacing on both the top 
     * and bottom.
     */
    readonly property real absoluteMaximumHeight: Screen.height - Kirigami.Units.gridUnit * 2
    
    /**
     * This property holds the absolute maximum width the dialog can be.
     * 
     * By default, it is the window width, subtracted by largeSpacing on both 
     * the top and bottom.
     */
    readonly property real absoluteMaximumWidth: Screen.width - Kirigami.Units.gridUnit * 2
    
    /**
     * This property holds the maximum height the dialog can be (including
     * the header and footer).
     * 
     * The height restriction is solely enforced on the content, so if the
     * maximum height given is not larger than the height of the header and
     * footer, it will be ignored.
     * 
     * By default, this is `absoluteMaximumHeight`.
     */
    property real maximumHeight: absoluteMaximumHeight
    
    /**
     * This property holds the maximum width the dialog can be.
     * 
     * By default, this is `absoluteMaximumWidth`.
     */
    property real maximumWidth: absoluteMaximumWidth
    
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

    property bool darkenBackground: true
    
    property bool showCloseButton: false
    
    property real dialogCornerRadius: Kirigami.Units.smallSpacing * 2
    
    width: loader.item.implicitWidth
//     height: loader.item.implicitHeight

    signal opened()
    signal closed()

    onVisibleChanged: {
        if (visible) {
            root.opened();
        } else {
            root.closed();
        }
    }
    
    // load in async to speed up load times (especially on embedded devices)
    Loader {
        id: loader
        anchors.fill: parent
        asynchronous: true
        
        sourceComponent: Item {
            // margins for shadow
            implicitWidth: control.implicitWidth + Kirigami.Units.gridUnit
            implicitHeight: control.implicitHeight + Kirigami.Units.gridUnit
            
            // shadow
            RectangularGlow {
                id: glow
                anchors.topMargin: 1 
                anchors.fill: control
                cached: true
                glowRadius: 2
                cornerRadius: Kirigami.Units.gridUnit
                spread: 0.1
                color: Qt.rgba(0, 0, 0, 0.4)
            }
            
            // actual window
            Control {
                id: control
                anchors.centerIn: parent
                topPadding: 0
                bottomPadding: 0
                rightPadding: 0
                leftPadding: 0
            
                background: Item {
                    Rectangle { // border
                        anchors.fill: parent
                        anchors.margins: -1
                        radius: dialogCornerRadius + 1
                        color: Qt.darker(Kirigami.Theme.backgroundColor, 1.5)
                    }
                    Rectangle { // background colour
                        anchors.fill: parent
                        radius: dialogCornerRadius
                        color: Kirigami.Theme.backgroundColor
                    }
                    
                    Kirigami.Icon { // close button
                        id: closeIcon
                        z: 1
                        anchors.top: parent.top
                        anchors.right: parent.right
                        anchors.margins: Kirigami.Units.smallSpacing * 2
                        visible: root.showCloseButton
                        
                        implicitHeight: Kirigami.Units.iconSizes.smallMedium
                        implicitWidth: implicitHeight
                        
                        source: closeMouseArea.containsMouse ? "window-close" : "window-close-symbolic"
                        active: closeMouseArea.containsMouse
                        MouseArea {
                            id: closeMouseArea
                            hoverEnabled: Qt.styleHints.useHoverEffects
                            anchors.fill: parent
                            onClicked: root.close()
                        }
                    }
                }
                
                contentItem: root.bodyItem
            }
        }
    }
}
