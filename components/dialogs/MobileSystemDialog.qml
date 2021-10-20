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

Private.AbstractSystemDialog {
    id: root
    
    default property Item mainItem
    
    /**
     * Title of the dialog.
     */
    property string title: ""
    
    /**
     * Subtitle of the dialog.
     */
    property string subtitle: ""
    
    /**
     * This property holds the default padding of the content.
     */
    property real padding: Kirigami.Units.smallSpacing
    
    /**
     * This property holds the left padding of the content. If not specified, it uses `padding`.
     */
    property real leftPadding: padding
    
    /**
     * This property holds the right padding of the content. If not specified, it uses `padding`.
     */
    property real rightPadding: padding
    
    /**
     * This property holds the top padding of the content. If not specified, it uses `padding`.
     */
    property real topPadding: padding
    
    /**
     * This property holds the bottom padding of the content. If not specified, it uses `padding`.
     */
    property real bottomPadding: padding
    
    /**
     * This property holds the list of actions for this dialog.
     *
     * Each action will be rendered as a button that the user will be able
     * to click.
     */
    property list<Kirigami.Action> actions
    
    enum ActionLayout {
        Row,
        Column
    }
    
    /**
     * The layout of the action buttons in the footer of the dialog.
     * 
     * By default, if there are more than 3 actions, it will have `SystemDialog.Column`.
     * 
     * Otherwise, with zero to 2 actions, it will have `SystemDialog.Row`.
     */
    property int layout: actions.length > 3 ? 1 : 0
    
    bodyItem: ColumnLayout {
        spacing: 0
        
        // header
        Control {
            id: headerControl
            
            Layout.fillWidth: true
            Layout.maximumWidth: root.maximumWidth
            Layout.preferredWidth: root.preferredWidth
            
            topPadding: 0
            leftPadding: 0
            rightPadding: 0
            bottomPadding: 0
            
            background: Item {}
            
            contentItem: RowLayout {
                Kirigami.Heading {
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    Layout.bottomMargin: Kirigami.Units.largeSpacing
                    Layout.leftMargin: Kirigami.Units.largeSpacing
                    Layout.rightMargin: Kirigami.Units.largeSpacing
                    Layout.alignment: Qt.AlignVCenter
                    level: 2
                    text: root.title
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
        
        // content
        Control {
            id: content
            
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.maximumWidth: root.maximumWidth
            Layout.preferredWidth: root.preferredWidth
            
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            
            background: Item {}
            contentItem: ColumnLayout {
                spacing: 0
                
                Label {
                    id: subtitleLabel
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    Layout.bottomMargin: Kirigami.Units.largeSpacing
                    Layout.leftMargin: Kirigami.Units.gridUnit * 3
                    Layout.rightMargin: Kirigami.Units.gridUnit * 3
                    visible: root.subtitle !== ""
                    horizontalAlignment: Text.AlignHCenter
                    text: root.subtitle
                    wrapMode: Label.Wrap
                }
                
                // separator when scrolling
                Kirigami.Separator {
                    Layout.fillWidth: true
                    opacity: root.mainItem && contentControl.flickableItem && contentControl.flickableItem.contentY !== 0 ? 1 : 0 // always maintain same height (as opposed to visible)
                }
                
                // mainItem is in scrollview, in case of overflow
                Private.ScrollView {
                    id: contentControl

                    // we cannot have contentItem inside a sub control (allowing for content padding within the scroll area),
                    // because if the contentItem is a Flickable (ex. ListView), the ScrollView needs it to be top level in order
                    // to decorate it
                    contentItem: root.mainItem
                    canFlickWithMouse: true

                    // ensure window colour scheme, and background color
                    Kirigami.Theme.inherit: false
                    Kirigami.Theme.colorSet: Kirigami.Theme.Window
                    
                    // needs to explicitly be set for each side to work
                    leftPadding: root.leftPadding; topPadding: root.topPadding
                    rightPadding: root.rightPadding; bottomPadding: root.bottomPadding
                    
                    // height of everything else in the dialog other than the content
                    property real otherHeights: headerControl.height + subtitleLabel.height + footerControl.height + root.topPadding + root.bottomPadding;
                    
                    property real calculatedMaximumWidth: root.maximumWidth > root.absoluteMaximumWidth ? root.absoluteMaximumWidth : root.maximumWidth
                    property real calculatedMaximumHeight: root.maximumHeight > root.absoluteMaximumHeight ? root.absoluteMaximumHeight : root.maximumHeight
                    property real calculatedImplicitWidth: root.mainItem ? (root.mainItem.implicitWidth ? root.mainItem.implicitWidth : root.mainItem.width) + root.leftPadding + root.rightPadding : 0
                    property real calculatedImplicitHeight: root.mainItem ? (root.mainItem.implicitHeight ? root.mainItem.implicitHeight : root.mainItem.height) + root.topPadding + root.bottomPadding : 0
                    
                    // don't enforce preferred width and height if not set
                    Layout.preferredWidth: root.preferredWidth >= 0 ? root.preferredWidth : calculatedImplicitWidth + contentControl.rightSpacing
                    Layout.preferredHeight: root.preferredHeight >= 0 ? root.preferredHeight - otherHeights : calculatedImplicitHeight + contentControl.bottomSpacing
                    
                    Layout.fillWidth: true
                    Layout.maximumWidth: calculatedMaximumWidth
                    Layout.maximumHeight: calculatedMaximumHeight >= otherHeights ? calculatedMaximumHeight - otherHeights : 0 // we enforce maximum height solely from the content
                    
                    // give an implied width and height to the contentItem so that features like word wrapping/eliding work
                    // cannot placed directly in contentControl as a child, so we must use a property
                    property var widthHint: Binding {
                        target: root.mainItem
                        property: "width"
                        // we want to avoid horizontal scrolling, so we apply maximumWidth as a hint if necessary
                        property real preferredWidthHint: contentControl.Layout.preferredWidth - root.leftPadding - root.rightPadding - contentControl.rightSpacing
                        property real maximumWidthHint: contentControl.calculatedMaximumWidth - root.leftPadding - root.rightPadding - contentControl.rightSpacing
                        value: maximumWidthHint < preferredWidthHint ? maximumWidthHint : preferredWidthHint
                    }
                    property var heightHint: Binding {
                        target: root.mainItem
                        property: "height"
                        // we are okay with overflow, if it exceeds maximumHeight we will allow scrolling
                        value: contentControl.Layout.preferredHeight - root.topPadding - root.bottomPadding - contentControl.bottomSpacing
                    }
                    
                    // give explicit warnings since the maximumHeight is ignored when negative, so developers aren't confused
                    Component.onCompleted: {
                        if (contentControl.Layout.maximumHeight < 0 || contentControl.Layout.maximumHeight === Infinity) {
                            console.log("Dialog Warning: the calculated maximumHeight for the content is " + contentControl.Layout.maximumHeight + ", ignoring...");
                        }
                    }
                }
            }
        }
        
        // footer
        Control {
            id: footerControl
            visible: root.actions.length > 0
            
            Layout.fillWidth: true
            Layout.maximumWidth: root.maximumWidth
            Layout.preferredWidth: root.preferredWidth
            
            topPadding: 0
            bottomPadding: 0
            leftPadding: 0
            rightPadding: 0
            
            background: Kirigami.ShadowedRectangle {
                Kirigami.Theme.colorSet: Kirigami.Theme.Window
                Kirigami.Theme.inherit: false
                color: Kirigami.Theme.backgroundColor
                corners.bottomRightRadius: root.dialogCornerRadius
                corners.bottomLeftRadius: root.dialogCornerRadius
            } 
            
            contentItem: ColumnLayout {
                spacing: 0
                
                // footer buttons (horizontal layout)
                Component {
                    id: horizontalButtons
                    RowLayout {
                        id: horizontalRowLayout
                        spacing: 0
                        
                        Repeater {
                            model: actions
                            
                            delegate: RowLayout {
                                spacing: 0
                                Layout.fillHeight: true
                                
                                Kirigami.Separator {
                                    id: separator
                                    property real fullWidth: width
                                    visible: model.index !== 0
                                    Layout.fillHeight: true
                                }
                                
                                Private.MobileSystemDialogButton {
                                    Layout.fillWidth: true
                                    // ensure consistent widths for all buttons
                                    Layout.maximumWidth: (horizontalRowLayout.width - Math.max(0, root.actions.length - 1) * separator.fullWidth) / root.actions.length
                                    
                                    corners.bottomLeftRadius: model.index === 0 ? root.dialogCornerRadius : 0
                                    corners.bottomRightRadius: model.index === root.actions.length - 1 ? root.dialogCornerRadius : 0
                                    
                                    text: modelData.text
                                    icon: modelData.icon
                                    onClicked: modelData.trigger()
                                }
                            }
                        }
                    }
                }
                
                // footer buttons (column layout)
                Component {
                    id: verticalButtons
                    ColumnLayout {
                        spacing: 0
                        
                        Repeater {
                            model: actions
                            
                            delegate: ColumnLayout {
                                spacing: 0
                                Layout.fillWidth: true
                                
                                Kirigami.Separator {
                                    visible: model.index !== 0
                                    Layout.fillWidth: true
                                }
                                
                                Private.MobileSystemDialogButton {
                                    Layout.fillWidth: true
                                    corners.bottomLeftRadius: model.index === root.actions.length - 1 ? root.dialogCornerRadius : 0
                                    corners.bottomRightRadius: model.index === root.actions.length - 1 ? root.dialogCornerRadius : 0
                                    text: modelData.text
                                    icon: modelData.icon
                                    onClicked: modelData.trigger()
                                }
                            }
                        }
                    }
                }
                
                // actual buttons loader
                Kirigami.Separator {
                    Layout.fillWidth: true
                }
                Loader {
                    Layout.fillWidth: true
                    active: true
                    sourceComponent: layout === 0 ? horizontalButtons : verticalButtons
                }
            }
        }
    }
}
