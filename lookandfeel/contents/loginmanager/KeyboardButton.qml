import QtQuick 2.2

import QtQuick.Controls 1.1 //for Menu

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

PlasmaComponents.ToolButton {
    id: keyboardButton

    property int currentIndex: -1

    text: menu.items[currentIndex].text
    implicitWidth: minimumWidth

    //if we bind directly we would set text, before we've populated the menu which it gets the text from
    Component.onCompleted: currentIndex = Qt.binding(function() {return keyboard.currentLayout});

    menu: Menu {
        id: menu
        Instantiator {
            model: keyboard.layouts
            delegate: MenuItem {
                text: modelData.shortName
                onTriggered: keyboard.currentLayout = model.index

            }
            onObjectAdded: menu.insertItem(index, object);
        }
    }
}
