import QtQuick 2.4
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.1

import org.kde.plasma.core 2.0 as Plasmacore

RowLayout {
    property alias cfg_showMediaControls: showMediaControls.checked

    spacing: units.largeSpacing / 2

    Label {
        Layout.minimumWidth: formAlignment - units.largeSpacing //to match wallpaper config...
        horizontalAlignment: Text.AlignRight
        text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Show media controls:")
    }
    CheckBox {
        id: showMediaControls
    }
    Item {
        Layout.fillWidth: true
    }
}
