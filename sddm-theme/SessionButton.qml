import QtQuick 2.2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.ToolButton {
    id: root
    property int currentIndex: -1

    implicitWidth: minimumWidth

    iconSource: ""
    text: menu.content[currentIndex].text

    onClicked: menu.open()

    PlasmaComponents.ContextMenu {
        id: menu
        visualParent: root

        property Item _children : Item {
            Repeater {
                model: sessionModel
                id: repeater
                delegate: PlasmaComponents.MenuItem {
                    text: model.name
//                             icon:
                    onClicked: {
                        root.currentIndex = model.index
                        console.log(model.index)
                    }
                    Component.onCompleted: {
                        parent = menu
                    }
                }
            }
        }
    }
}
