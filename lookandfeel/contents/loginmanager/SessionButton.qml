import QtQuick 2.2
import QtQml 2.2

import QtQuick.Controls 1.1 //for Menu

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.ToolButton {
    id: root
    property int currentIndex: -1

    implicitWidth: minimumWidth

    iconSource: ""
    text: menu.items[currentIndex].text

    menu: Menu {
        Instantiator {
            model: sessionModel
            delegate: MenuItem {
                text: model.name
                onTriggered: root.currentIndex = model.index

            }
            onObjectAdded: menu.insertItem(index, object);
        }
    }
}
