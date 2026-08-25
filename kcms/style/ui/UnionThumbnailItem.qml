// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.union as Union

Union.Pane {
    id: root

    property string styleId

    signal selected()

    Union.Element.styleId: styleId

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Union.TabBar {
            Union.TabButton {
                text: i18nc("@title:tab", "Tab 1")
            }

            Union.TabButton {
                text: i18nc("@title:tab", "Tab 2")
            }
        }

        Union.Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TapHandler {
                onTapped: root.selected()
            }

            GridLayout {
                anchors.fill: parent

                columnSpacing: Kirigami.Units.smallSpacing
                rowSpacing: Kirigami.Units.smallSpacing
                flow: GridLayout.TopToBottom

                Union.CheckBox {
                    Layout.fillWidth: true
                    text: i18nc("@option:check", "Checkbox")
                    checked: true
                }

                Union.RadioButton {
                    Layout.fillWidth: true
                    text: i18nc("@option:radio", "Radio Button")
                    checked: true
                }

                Union.RadioButton {
                    Layout.fillWidth: true
                    text: i18nc("@option:radio", "Radio Button")
                }

                Union.Slider {
                    id: slider

                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: 70
                    stepSize: 10

                    wheelEnabled: false
                }

                Union.ComboBox {
                    Layout.column: 2
                    Layout.fillWidth: true

                    wheelEnabled: false

                    model: [
                        i18nc("@item:inlistbox", "Combo Box"),
                        i18nc("@item:inlistbox", "Combo Box Item")
                    ]
                }

                RowLayout {
                    spacing: Kirigami.Units.smallSpacing

                    Union.Button {
                        Layout.fillWidth: true
                        text: i18nc("@action:button", "Button")
                    }

                    Union.SpinBox {
                        Layout.fillWidth: true
                        Layout.minimumWidth: implicitWidth
                        wheelEnabled: false
                    }
                }

                Union.TextField {
                    Layout.fillWidth: true

                    placeholderText: i18nc("@label:textbox", "Text Field")
                }

                RowLayout {
                    Union.ProgressBar {
                        id: progressBar

                        Layout.fillWidth: true

                        from: 0
                        to: 100
                        value: slider.value
                    }

                    Union.Label {
                        text: i18nc("@item:inrange Progress bar value", "%1%", progressBar.value)
                    }
                }

                Union.ScrollBar {
                    Layout.column: 3
                    Layout.rowSpan: 4
                    Layout.fillHeight: true

                    orientation: Qt.Vertical
                    size: 0.25
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: root.selected()
    }
}
