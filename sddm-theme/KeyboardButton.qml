import QtQuick 2.6

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents

import QtQuick.Controls 2.5 as QQC2
import QtGraphicalEffects 1.0

PlasmaComponents.ToolButton {
    id: keyboardButton

    property int currentIndex: -1

    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Keyboard Layout: %1", instantiator.objectAt(currentIndex).shortName)

    font.pointSize: config.fontSize

    visible: keyboardMenu.count > 1

    Component.onCompleted: currentIndex = Qt.binding(function() {return keyboard.currentLayout});

    onClicked: {
        keyboardMenu.popup(x, y)
    }

    QQC2.Menu {
        id: keyboardMenu

        property int largestWidth: 9999

        Component.onCompleted: {
            var trueWidth = 0;
            for (var i = 0; i < keyboardMenu.count; i++) {
                trueWidth = Math.max(trueWidth, keyboardMenu.itemAt(i).textWidth)
            }
            keyboardMenu.largestWidth = trueWidth
        }

        background: Rectangle {
            implicitHeight: 40
            implicitWidth: keyboardMenu.largestWidth > keyboardButton.implicitWidth ? keyboardMenu.largestWidth : keyboardButton.implicitWidth
            color: PlasmaCore.ColorScope.backgroundColor
            property color borderColor: PlasmaCore.Colorscope.textColor
            border.color: Qt.rgba(borderColor.r, borderColor.g, borderColor.b, 0.3)
            border.width: 1
            layer.enabled: true
            layer.effect: DropShadow {
                transparentBorder: true
                radius: 8
                samples: 8
                horizontalOffset: 0
                verticalOffset: 2
                color: Qt.rgba(0, 0, 0, 0.3)
            }
        }

        Instantiator {
            id: instantiator
            model: keyboard.layouts
            onObjectAdded: keyboardMenu.addItem( object )
            onObjectRemoved: keyboardMenu.removeItem( object )
            delegate: QQC2.MenuItem {
                id: menuItem
                property string name: modelData.longName
                property string shortName: modelData.shortName

                property real textWidth: text.contentWidth + 20
                implicitWidth: text.contentWidth + 20
                implicitHeight: Math.round(text.contentHeight * 1.6)

                contentItem: QQC2.Label {
                    id: text
                    font.pointSize: config.fontSize
                    text: modelData.longName
                    color: menuItem.highlighted ? PlasmaCore.ColorScope.highlightedTextColor : PlasmaCore.ColorScope.textColor
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                background: Rectangle {
                    color: PlasmaCore.ColorScope.highlightColor
                    opacity: menuItem.highlighted ? 1 : 0
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onContainsMouseChanged: menuItem.highlighted = containsMouse
                        onClicked: {
                            keyboardMenu.dismiss()
                            keyboard.currentLayout = model.index
                        }
                    }
                }
            }
        }

        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 150
            }
        }
        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 150
            }
        }
    }
}
