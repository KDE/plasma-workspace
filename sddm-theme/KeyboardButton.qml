import QtQuick 2.2

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // Because PC3 ToolButton can't take a menu

import QtQuick.Controls 1.3 as QQC

PlasmaComponents.ToolButton {
    id: keyboardButton

    property int currentIndex: -1

    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Keyboard Layout: %1", keyboard.layouts[currentIndex].shortName)
    implicitWidth: minimumWidth

    visible: menu.items.length > 1

    Component.onCompleted: currentIndex = Qt.binding(function() {return keyboard.currentLayout});

    menu: QQC.Menu {
        id: keyboardMenu
        style: BreezeMenuStyle {}
        Instantiator {
            id: instantiator
            model: keyboard.layouts
            onObjectAdded: keyboardMenu.insertItem(index, object)
            onObjectRemoved: keyboardMenu.removeItem( object )
            delegate: QQC.MenuItem {
                text: modelData.longName
                onTriggered: {
                    keyboard.currentLayout = model.index
                }
            }
        }
    }
}
