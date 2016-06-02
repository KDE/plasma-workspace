/*
 * Copyright 2013  Bhushan Shah <bhush94@gmail.com>
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

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtGraphicalEffects 1.0

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components
import org.kde.kquickcontrolsaddons 2.0
import org.kde.draganddrop 2.0 as DragDrop
import org.kde.plasma.private.icon 1.0

MouseArea {
    id: root

    property bool containsAcceptableDrag: false
    property int jumpListCount: 0

    height: units.iconSizes.desktop + theme.mSize(theme.defaultFont).height
    width: units.iconSizes.desktop

    Layout.minimumWidth: formFactor === PlasmaCore.Types.Horizontal ? height : units.iconSizes.small
    Layout.minimumHeight: formFactor === PlasmaCore.Types.Vertical ? width  : units.iconSizes.small
    property int formFactor: plasmoid.formFactor
    property bool constrained: formFactor == PlasmaCore.Types.Vertical || formFactor == PlasmaCore.Types.Horizontal
    hoverEnabled: true
    onClicked: logic.open();

    Plasmoid.preferredRepresentation: Plasmoid.fullRepresentation
    Plasmoid.icon: logic.icon
    Plasmoid.title: logic.name
    Plasmoid.backgroundHints: PlasmaCore.Types.NoBackground

    Component.onCompleted: {
        plasmoid.activated.connect(logic.open);

        updateActions()
    }

    function updateActions() {
        plasmoid.removeAction("properties")
        plasmoid.removeAction("separator0")
        for (var i = 0, length = jumpListCount; i < length; ++i) {
            plasmoid.removeAction("jumplist_" + i)
        }

        var actions = logic.jumpListActions
        jumpListCount = actions.length
        for (var i = 0; i < jumpListCount; ++i) {
            var item = actions[i]
            plasmoid.setAction("jumplist_" + i, item.name, item.icon)
        }

        if (jumpListCount) {
            plasmoid.setActionSeparator("separator0")
        }
    }

    function actionTriggered(name) {
        if (name.indexOf("jumplist_") === 0) {
            var actionIndex = parseInt(name.substr("jumplist_".length))
            logic.execJumpList(actionIndex)
        }
    }

    DragDrop.DropArea {
        id: dropArea
        anchors.fill: parent
        preventStealing: true
        onDragEnter: root.containsAcceptableDrag = event.mimeData.hasUrls
        onDragLeave: root.containsAcceptableDrag = false
        onDrop: {
            logic.processDrop(event)
            root.containsAcceptableDrag = false
        }
    }

    PlasmaCore.IconItem {
        id:icon
        source: plasmoid.icon
        anchors{
            left : parent.left
            right : parent.right
            top : parent.top
            bottom : constrained ? parent.bottom : text.top
        }
        active: root.containsMouse || root.containsAcceptableDrag
        usesPlasmaTheme: false
    }

    DropShadow {
        id: textShadow

        anchors.fill: text

        visible: !constrained

        horizontalOffset: units.devicePixelRatio * 2
        verticalOffset: horizontalOffset

        radius: 9.0
        samples: 18
        spread: 0.15

        color: "black"

        source: constrained ? null : text
    }

    Components.Label {
        id : text
        text : plasmoid.title
        anchors {
            left : parent.left
            bottom : parent.bottom
            right : parent.right
        }
        height: undefined // unset Label defaults
        horizontalAlignment : Text.AlignHCenter
        visible: false // rendered by DropShadow
        maximumLineCount: 2
        color: "white"
        elide: Text.ElideRight
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    }

    PlasmaCore.ToolTipArea {
        anchors.fill: parent
        mainText: logic.name
        subText: logic.genericName !== mainText ? logic.genericName :""
        icon: logic.icon
    }

    Logic {
        id: logic
        url: plasmoid.configuration.url
        onJumpListActionsChanged: updateActions()
    }

    Connections {
        target: plasmoid
        onExternalData: {
            plasmoid.configuration.url = data
        }
    }
}
