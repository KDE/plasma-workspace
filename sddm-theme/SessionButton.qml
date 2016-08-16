import QtQuick 2.2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import QtQuick.Controls 1.3 as QQC

PlasmaComponents.ToolButton {
    id: root
    property int currentIndex: -1

    implicitWidth: minimumWidth

    iconSource: ""
    text: instantiator.objectAt(currentIndex).text

    menu: QQC.Menu {
        id: menu
        Instantiator {
            id: instantiator
            model: sessionModel
            onObjectAdded: menu.insertItem(index, object)
            onObjectRemoved: menu.removeItem( object )
            delegate: QQC.MenuItem {
                text: model.name
                onTriggered: {
                    root.currentIndex = model.index
                }
            }
        }
    }
}
