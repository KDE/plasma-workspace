// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2026 Arjen Hiemstra <ahiemstra@quantumproductions.info>

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.union as Union

Pane {
    id: root

    property string styleId

    signal selected()

    Union.Element.styleId: styleId

    TapHandler {
        onTapped: root.selected()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            TabButton {
                text: "Tab 1"
            }

            TabButton {
                text: "Tab 2"
            }
        }

        Frame {
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

                CheckBox {
                    text: "Check Box"
                    checked: true
                }

                RadioButton {
                    text: "Radio Button"
                    checked: true
                }

                RadioButton {
                    text: "Radio Button"
                }

                Slider {
                    id: slider

                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: 70
                    stepSize: 10

                    Kirigami.StyleHints.tickMarkStepSize: 20
                }

                ComboBox {
                    Layout.column: 2

                    model: [
                        "Combo Box",
                        "Combo Box Item"
                    ]
                }

                RowLayout {
                    Button {
                        text: "Button"
                    }

                    SpinBox {

                    }
                }

                TextField {
                    placeholderText: "Text Field"
                }

                RowLayout {
                    ProgressBar {
                        id: progressBar

                        Layout.fillWidth: true

                        from: 0
                        to: 100
                        value: slider.value
                    }

                    Label {
                        text: progressBar.value + "%"
                    }
                }

                ScrollBar {
                    Layout.column: 3
                    Layout.rowSpan: 4
                    Layout.fillHeight: true

                    orientation: Qt.Vertical
                    size: 0.25
                }
            }
        }
    }
}
