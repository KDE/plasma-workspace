/*
    SPDX-FileCopyrightText: 2016 David Edmundson <davidedmundson@kde.org>
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts
import QtQuick.Shapes

import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.kirigami 2.20 as Kirigami
import org.kde.kitemmodels as KItemModels
import org.kde.ksvg as KSvg

PlasmaComponents.ToolButton {
    id: root

    property int currentIndex: -1

    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Desktop Session: %1", instantiator.objectAt(currentIndex).text || "")
    visible: menu.count > 1

    Component.onCompleted: {
        currentIndex = sessionModel.lastIndex
    }

    // unexported enum in sddm/Session.h
    property int typeWaylandSession: 2

    function indexOfFirstWaylandSession(): int {
        const m = sessionModel;
        const count = m.rowCount();
        for (let row = 0; row < count; row++) {
            const index = m.index(row, 0);
            const name = m.data(index, m.KItemModels.KRoleNames.role("name"));
            const type = m.data(index, m.KItemModels.KRoleNames.role("type"));

            if (type === typeWaylandSession) {
                return row;
            }
        }
        return -1;
    }

    PlasmaComponents.Popup {
        id: waylandRecommendationPopup

        property int waylandSessionIndex: -1

        property real animationYOffset: 0

        parent: root
        visible: false

        modal: false
        dim: false
        clip: false

        y: -height - Kirigami.Units.largeSpacing + animationYOffset
        width: Kirigami.Units.gridUnit * 20
        height: implicitHeight
        margins: Kirigami.Units.largeSpacing

        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.medium
                    Layout.preferredHeight: Kirigami.Units.iconSizes.medium
                    source: "wayland"
                }

                Kirigami.Heading {
                    text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Try Wayland Now!")
                    level: 2
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                }
            }

            PlasmaComponents.Label {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "If it doesn't work good enough, you can always switch back to X11, and we'll be happy to hear your feedback.")
                wrapMode: Text.Wrap
            }

            PlasmaComponents.Button {
                Layout.alignment: Qt.AlignRight
                text: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Try Now!")
                icon.name: "arrow-right"
                enabled: waylandRecommendationPopup.waylandSessionIndex !== -1
                onClicked: {
                    waylandRecommendationPopup.close();
                    if (waylandRecommendationPopup.waylandSessionIndex !== -1) {
                        root.currentIndex = waylandRecommendationPopup.waylandSessionIndex;
                    }
                }
            }
        }

        background: KSvg.FrameSvgItem {
            implicitWidth: Kirigami.Units.gridUnit * 12
            imagePath: "widgets/background"

            // Pointy arrow to the bottom, like a speech bubble.
            // Note: not styled with SVG theme!
            Shape {
                anchors {
                    top: parent.bottom
                    topMargin: -9
                    horizontalCenter: parent.horizontalCenter
                }
                width: 19
                height: 9

                ShapePath {
                    fillColor: Kirigami.Theme.backgroundColor
                    strokeColor: Qt.alpha(Kirigami.Theme.textColor, 0.3)
                    strokeWidth: 1

                    startX: 0
                    startY: 0
                    PathLine {
                        x: 9
                        y: 9
                    }
                    PathLine {
                        x: 18
                        y: 0
                    }
                }
            }
        }

        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                easing.type: Easing.InOutCubic
                duration: Kirigami.Units.longDuration
            }
            NumberAnimation {
                property: "animationYOffset"
                from: -50
                to: 0
                easing.type: Easing.InOutCubic
                duration: Kirigami.Units.longDuration
            }
        }

        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                easing.type: Easing.InOutCubic
                duration: Kirigami.Units.longDuration
            }
            NumberAnimation {
                property: "animationYOffset"
                from: 0
                to: -50
                easing.type: Easing.InOutCubic
                duration: Kirigami.Units.longDuration
            }
        }

        function setupPositionBinginds() {
            x = Qt.binding(() => Math.round((parent.width - width) / 2));
        }

        function breakPositionBindings() {
            x = x;
        }

        function maybeShow() {
            const m = sessionModel;
            const index = m.index(root.currentIndex, 0);
            const name = m.data(index, m.KItemModels.KRoleNames.role("name"));
            const type = m.data(index, m.KItemModels.KRoleNames.role("type"));

            if (type !== typeWaylandSession) {
                waylandSessionIndex = indexOfFirstWaylandSession();
                if (waylandSessionIndex !== -1) {
                    open();
                }
            }
        }

        onAboutToShow: setupPositionBinginds()
        onAboutToHide: breakPositionBindings()
    }

    // Note: window isn't immediately available at the component's creation time.
    Window.onWindowChanged: {
        if (Window.window) {
            Qt.callLater(waylandRecommendationPopup.maybeShow);
        }
    }

    checkable: true
    checked: menu.opened
    onToggled: {
        if (checked) {
            menu.popup(root, 0, 0)
        } else {
            menu.dismiss()
        }
    }

    signal sessionChanged()

    PlasmaComponents.Menu {
        Kirigami.Theme.colorSet: Kirigami.Theme.Window
        Kirigami.Theme.inherit: false

        id: menu
        Instantiator {
            id: instantiator
            model: sessionModel
            onObjectAdded: (index, object) => menu.insertItem(index, object)
            onObjectRemoved: (index, object) => menu.removeItem(object)
            delegate: PlasmaComponents.MenuItem {
                text: model.name
                onTriggered: {
                    root.currentIndex = model.index
                    sessionChanged()
                }
            }
        }
    }
}
