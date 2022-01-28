import QtQuick 2.15
// Don't bump them any further, these are the latest.
import QtQuick.Controls 1.6 as QQC
import QtQuick.Controls.Styles 1.4 as QQCS

import org.kde.plasma.core 2.0 as PlasmaCore

QQCS.MenuStyle {
    frame: Rectangle {
        color: PlasmaCore.ColorScope.backgroundColor
        border.color: Qt.tint(PlasmaCore.ColorScope.textColor, Qt.rgba(color.r, color.g, color.b, 0.7))
        border.width: 1
    }
    itemDelegate.label: QQC.Label {
        height: contentHeight * 1.2
        verticalAlignment: Text.AlignVCenter
        color: styleData.selected ? PlasmaCore.ColorScope.highlightedTextColor : PlasmaCore.ColorScope.textColor
        font.pointSize: config.fontSize
        text: styleData.text
    }
    itemDelegate.background: Rectangle {
        visible: styleData.selected
        color: PlasmaCore.ColorScope.highlightColor
    }
}
