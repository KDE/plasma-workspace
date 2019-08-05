import QtQuick 2.5
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Layouts 1.1

RowLayout {
    property alias cfg_showMediaControls: showMediaControls.checked

    spacing: units.largeSpacing / 2

    QQC2.Label {
        Layout.minimumWidth: formAlignment - units.largeSpacing //to match wallpaper config...
        horizontalAlignment: Text.AlignRight
        text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Media controls:")
    }
    QQC2.CheckBox {
        id: showMediaControls
        text: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "verb, to show something", "Show")
    }
    Item {
        Layout.fillWidth: true
    }
}
