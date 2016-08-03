import QtQuick 2.2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.ToolButton {
    id: keyboardButton

    property int currentIndex: -1

    text: keyboardMenu.content[currentIndex].shortName
    implicitWidth: minimumWidth

    onClicked: keyboardMenu.open()

    Component.onCompleted: currentIndex = Qt.binding(function() {return keyboard.currentLayout});

    PlasmaComponents.ContextMenu {
        id: keyboardMenu
        visualParent: keyboardButton

        property Item _children : Item {
            Repeater {
                model: keyboard.layouts
                delegate: PlasmaComponents.MenuItem {
                    text: modelData.longName
                    property string shortName: modelData.shortName
//                             icon:
                    onClicked: {
                        keyboard.currentLayout = model.index
                    }
                    Component.onCompleted: {
                        parent = keyboardMenu
                    }
                }
            }
        }
    }
}
