/*
 *  SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */
import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQml 2.15
import org.kde.kirigami 2.9 as Kirigami

// taken from Kirigami
MouseArea {
    id: root
    default property Item contentItem
    property Flickable flickableItem
    clip: true

    property int horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
    property int verticalScrollBarPolicy: Qt.ScrollBarAsNeeded
    property int topPadding: 0
    property int leftPadding: 0
    property int bottomPadding: 0
    property int rightPadding: 0
    property bool canFlickWithMouse: false

    // Note: These are used because RefreshableScrollView overrides right and
    // bottom padding properties.
    property int rightSpacing: !Kirigami.Settings.hasTransientTouchInput && flickableItem && flickableItem.ScrollBar.vertical && flickableItem.ScrollBar.vertical.visible ? flickableItem.ScrollBar.vertical.width : 0
    property int bottomSpacing: !Kirigami.Settings.hasTransientTouchInput && flickableItem && flickableItem.ScrollBar.horizontal && flickableItem.ScrollBar.horizontal.visible ? flickableItem.ScrollBar.horizontal.height : 0

    Accessible.onScrollDownAction: flickableItem.Accessible.onScrollDownAction
    Accessible.onScrollUpAction: flickableItem.Accessible.onScrollUpAction

    Keys.onUpPressed: scroll(Kirigami.Units.gridUnit * 2)
    Keys.onDownPressed: scroll(-Kirigami.Units.gridUnit * 2)

    function scroll(y) {
        // Don't scroll if the view isn't scrollable!
        if (flickableItem.contentHeight < flickableItem.height + flickableItem.contentY) {
            return;
        }
        const minYExtent = flickableItem.topMargin - flickableItem.originY;
        const maxYExtent = flickableItem.height - (flickableItem.contentHeight + flickableItem.bottomMargin + flickableItem.originY);

        flickableItem.contentY = Math.min(-maxYExtent, Math.max(-minYExtent, flickableItem.contentY - y));
    }

    focus: true

    onPressed: mouse.accepted = mouse.source !== Qt.MouseEventNotSynthesized
    onVerticalScrollBarPolicyChanged: {
        scrollBarCreationTimer.restart();
    }
    onHorizontalScrollBarPolicyChanged: {
        scrollBarCreationTimer.restart();
    }


    onContentItemChanged: {
        if (contentItem.hasOwnProperty("contentY")) {
            flickableItem = contentItem;
            if (typeof(flickableItem.keyNavigationEnabled) != "undefined") {
                flickableItem.keyNavigationEnabled = true;
                flickableItem.keyNavigationWraps = false;
            }
            contentItem.parent = flickableParent;
        } else {
            flickableItem = flickableComponent.createObject(flickableParent);
            contentItem.parent = flickableItem.contentItem;
        }

        flickableItem.anchors.fill = flickableParent;

        scrollBarCreationTimer.restart();
    }

    Binding {
        when: !root.canFlickWithMouse
        target: root.flickableItem
        property: "interactive"
        value: Kirigami.Settings.hasTransientTouchInput
        restoreMode: Binding.RestoreBinding
    }
    Timer {
        id: scrollBarCreationTimer
        interval: 0
        onTriggered: {
            //create or destroy the vertical scrollbar
            if ((!flickableItem.ScrollBar.vertical) &&
                verticalScrollBarPolicy != Qt.ScrollBarAlwaysOff) {
                flickableItem.ScrollBar.vertical = verticalScrollComponent.createObject(root);
            } else if (flickableItem.ScrollBar.vertical &&
                verticalScrollBarPolicy == Qt.ScrollBarAlwaysOff) {
                flickableItem.ScrollBar.vertical.destroy();
            }

            //create or destroy the horizontal scrollbar
            if ((!flickableItem.ScrollBar.horizontal) &&
                horizontalScrollBarPolicy != Qt.ScrollBarAlwaysOff) {
                flickableItem.ScrollBar.horizontal = horizontalScrollComponent.createObject(root);
            } else if (flickableItem.ScrollBar.horizontal &&
                horizontalScrollBarPolicy == Qt.ScrollBarAlwaysOff) {
                flickableItem.ScrollBar.horizontal.destroy();
            }
        }
    }
    Kirigami.WheelHandler {
        id: wheelHandler
        target: root.flickableItem
    }
    Item {
        id: flickableParent
        clip: true
        anchors {
            fill: parent
            leftMargin: root.leftPadding
            topMargin: root.topPadding
            rightMargin: root.rightPadding + root.rightSpacing
            bottomMargin: root.bottomPadding + root.bottomSpacing
        }
    }
    Component {
        id: flickableComponent
        Flickable {
            anchors {
                fill: parent
            }
            contentWidth: root.contentItem ? root.contentItem.width : 0
            contentHeight: root.contentItem ? root.contentItem.height : 0
        }
    }
    Component {
        id: verticalScrollComponent
        ScrollBar {
            z: flickableParent.z + 1
            visible: root.verticalScrollBarPolicy != Qt.ScrollBarAlwaysOff && root.contentItem.visible && size < 1
            interactive: !Kirigami.Settings.hasTransientTouchInput

            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
                bottomMargin: root.bottomSpacing
            }
        }
    }
    Component {
        id: horizontalScrollComponent
        ScrollBar {
            z: flickableParent.z + 1
            visible: root.horizontalScrollBarPolicy != Qt.ScrollBarAlwaysOff && root.contentItem.visible && size < 1
            interactive: !Kirigami.Settings.hasTransientTouchInput

            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                rightMargin: root.rightSpacing
            }
        }
    }
}
