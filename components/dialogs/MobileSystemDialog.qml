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

Item {
    id: root
    
    default property Item mainItem
    
    /**
     * Title of the dialog.
     */
    property string mainText: ""
    
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
    property alias standardButtons: footerButtonBox.standardButtons

    readonly property int flags: Qt.FramelessWindowHint | Qt.Dialog
    readonly property real dialogCornerRadius: Kirigami.Units.smallSpacing * 2
    property list<Kirigami.Action> actions
    property string iconName

    implicitWidth: loader.implicitWidth
    implicitHeight: loader.implicitHeight

    readonly property real minimumHeight: implicitWidth
    readonly property real minimumWidth: implicitHeight

    required property Kirigami.AbstractApplicationWindow window

    function present() {
        root.window.showFullScreen()
    }

    onWindowChanged: {
        window.color = Qt.binding(() => {
            return Qt.rgba(0, 0, 0, 0.5)
        })
    }

    // load in async to speed up load times (especially on embedded devices)
    Loader {
        id: loader
        anchors.centerIn: parent
        asynchronous: true

        sourceComponent: Item {
            // margins for shadow
            implicitWidth: Math.min(Screen.width, control.implicitWidth + 2 * Kirigami.Units.gridUnit)
            implicitHeight: Math.min(Screen.height, control.implicitHeight + 2 * Kirigami.Units.gridUnit)

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
                anchors.fill: parent
                anchors.margins: glow.cornerRadius
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
                }

                contentItem: column
            }
        }
    }
    
    readonly property var contents: ColumnLayout {
        id: column
        spacing: 0

        // header
        Control {
            id: headerControl

            Layout.fillWidth: true
            Layout.maximumWidth: root.window.maximumWidth

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
                    text: root.mainText
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
            Layout.maximumWidth: root.window.maximumWidth

            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0

            background: Item {}
            contentItem: ColumnLayout {
                spacing: 0
                clip: true

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
                    clip: true

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
                    property real otherHeights: headerControl.height + subtitleLabel.height + footerButtonBox.height + root.topPadding + root.bottomPadding;

                    property real calculatedMaximumWidth: root.window.maximumWidth > root.absoluteMaximumWidth ? root.absoluteMaximumWidth : root.window.maximumWidth
                    property real calculatedMaximumHeight: root.window.maximumHeight > root.absoluteMaximumHeight ? root.absoluteMaximumHeight : root.window.maximumHeight
                    property real calculatedImplicitWidth: root.mainItem ? (root.mainItem.implicitWidth ? root.mainItem.implicitWidth : root.mainItem.width) + root.leftPadding + root.rightPadding : 0
                    property real calculatedImplicitHeight: root.mainItem ? (root.mainItem.implicitHeight ? root.mainItem.implicitHeight : root.mainItem.height) + root.topPadding + root.bottomPadding : 0

                    // don't enforce preferred width and height if not set
                    Layout.preferredWidth: root.preferredWidth >= 0 ? root.preferredWidth : calculatedImplicitWidth + contentControl.rightSpacing
                    Layout.preferredHeight: root.preferredHeight >= 0 ? root.preferredHeight - otherHeights : calculatedImplicitHeight + contentControl.bottomSpacing

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.maximumWidth: calculatedMaximumWidth
                    Layout.maximumHeight: calculatedMaximumHeight >= otherHeights ? calculatedMaximumHeight - otherHeights : 0 // we enforce maximum height solely from the content

                    // give an implied width and height to the contentItem so that features like word wrapping/eliding work
                    // cannot placed directly in contentControl as a child, so we must use a property
                    property var widthHint: Binding {
                        target: root.mainItem
                        property: "width"
                        // we want to avoid horizontal scrolling, so we apply maximumWidth as a hint if necessary
                        property real preferredWidthHint: contentControl.width - root.leftPadding - root.rightPadding - contentControl.rightSpacing
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
        Control {
            Layout.fillWidth: true

            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            contentItem: footerButtonBox
        }
    }

    readonly property DialogButtonBox dialogButtonBox: DialogButtonBox {
        //We want to report the same width on all so the button area is split equally
        id: footerButtonBox
        Layout.fillWidth: true
        onAccepted: root.window.accept()
        onRejected: root.window.reject()
        implicitHeight: contentItem.implicitHeight
        alignment: undefined

        readonly property real sameWidth: 50
        delegate: Private.MobileSystemDialogButton {
            Layout.fillWidth: true
            Layout.preferredWidth: footerButtonBox.sameWidth

            readonly property point globalPos: root.window.visible ? mapToItem(footerButtonBox, Qt.point(x, y)) : Qt.point(0, 0)
            verticalSeparator: globalPos.x > 0 && root.window.layout === Qt.Horizontal
            horizontalSeparator: true
            corners.bottomLeftRadius: verticalSeparator ? 0 : root.dialogCornerRadius
            corners.bottomRightRadius: globalPos.x >= footerButtonBox.width ? root.dialogCornerRadius : 0
        }

        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        contentItem: GridLayout {
            Layout.fillWidth: true

            rowSpacing: 0
            columnSpacing: 0
            rows: root.window.layout === Qt.Vertical ? Number.MAX_VALUE : 1
            columns: root.window.layout !== Qt.Vertical ? Number.MAX_VALUE : 1

            Repeater {
                model: root.actions
                delegate: Private.MobileSystemDialogButton {
                    Layout.fillWidth: true
                    Layout.preferredWidth: footerButtonBox.sameWidth
                    readonly property point globalPos: root.window.visible ? mapToItem(footerButtonBox, Qt.point(x, y)) : Qt.point(0, 0)
                    verticalSeparator: globalPos.x > 0 && root.window.layout === Qt.Horizontal
                    horizontalSeparator: true
                    corners.bottomLeftRadius: model.index === root.actions.length - 1 ? root.dialogCornerRadius : 0
                    corners.bottomRightRadius: model.index === root.actions.length - 1 && footerButtonBox.standardButtons === 0 ? root.dialogCornerRadius : 0
                    action: modelData
                }
            }
        }
    }
}
