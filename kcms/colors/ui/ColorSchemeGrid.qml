/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

KCM.GridDelegate {
        id: delegate

        enum Type {
            Light,
            Dark
        }

        required property int type

        text: model.display

        thumbnailAvailable: true
        thumbnail: Rectangle {
            anchors.fill: parent

            opacity: model.pendingDeletion ? 0.3 : 1
            Behavior on opacity {
                NumberAnimation { duration: Kirigami.Units.longDuration }
            }

            color: kcm.tinted(model.palette.window, kcm.accentColor, model.tints, model.tintFactor)

            Kirigami.Theme.inherit: false
            Kirigami.Theme.highlightColor: root.accentColor || model.palette.highlight
            Kirigami.Theme.textColor: kcm.tinted(model.palette.text, kcm.accentColor, model.tints, model.tintFactor)

            Rectangle {
                id: windowTitleBar
                width: parent.width
                height: Math.round(Kirigami.Units.gridUnit * 1.5)
                color: kcm.tinted((model.accentActiveTitlebar && root.accentColor) ? kcm.accentBackground(root.accentColor, model.palette.window) : model.activeTitleBarBackground, kcm.accentColor, model.tints, model.tintFactor)

                QtControls.Label {
                    anchors {
                        fill: parent
                        leftMargin: Kirigami.Units.smallSpacing
                        rightMargin: Kirigami.Units.smallSpacing
                    }
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: (model.accentActiveTitlebar && root.accentColor) ? kcm.accentForeground(kcm.accentBackground(root.accentColor, model.palette.window), true) : model.activeTitleBarForeground
                    text: i18n("Window Title")
                    elide: Text.ElideRight
                }
            }

            ColumnLayout {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: windowTitleBar.bottom
                    bottom: parent.bottom
                    margins: Kirigami.Units.smallSpacing
                }
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true

                    QtControls.Label {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        verticalAlignment: Text.AlignVCenter
                        text: i18n("Window text")
                        elide: Text.ElideRight
                        color: model.palette.windowText
                    }

                    QtControls.Button {
                        Layout.alignment: Qt.AlignBottom
                        text: i18n("Button")
                        Kirigami.Theme.inherit: false
                        Kirigami.Theme.highlightColor: kcm.tinted(root.accentColor ? kcm.accentBackground(root.accentColor, model.palette.base) : model.palette.highlight, kcm.accentColor, model.tints, model.tintFactor)
                        Kirigami.Theme.backgroundColor: kcm.tinted(model.palette.button, kcm.accentColor, model.tints, model.tintFactor)
                        Kirigami.Theme.textColor: kcm.tinted(model.palette.buttonText, kcm.accentColor, model.tints, model.tintFactor)
                        activeFocusOnTab: false
                    }
                }

                QtControls.Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 0

                    activeFocusOnTab: false

                    // Frame by default has a transparent background, override it so we can use the view color
                    // instead.
                    background: Rectangle {
                        color: Kirigami.Theme.backgroundColor
                        border.width: 1
                        border.color: kcm.tinted(Qt.rgba(model.palette.text.r, model.palette.text.g, model.palette.text.b, 0.3), kcm.accentColor, model.tints, model.tintFactor)
                    }

                    // We need to set inherit to false here otherwise the child ItemDelegates will not use the
                    // alternative base color we set here.
                    Kirigami.Theme.inherit: false
                    Kirigami.Theme.backgroundColor: kcm.tinted(model.palette.base, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.disabledTextColor: kcm.tinted(model.disabledText, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.highlightColor: kcm.tinted(root.accentColor ? kcm.accentBackground(root.accentColor, model.palette.base) : model.palette.highlight, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.highlightedTextColor: kcm.tinted(root.accentColor ? kcm.accentForeground(kcm.accentBackground(root.accentColor, model.palette.base), true) : model.palette.highlightedText, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.linkColor: kcm.tinted(root.accentColor || model.palette.link, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.textColor: kcm.tinted(model.palette.text, kcm.accentColor, model.tints, model.tintFactor)

                    Column {
                        id: listPreviewColumn

                        function demoText(palette) {
                            return " <a href='#'><font color='%1'>%2</font></a> <a href='#'><font color='%3'>%4</font></a>"
                            .arg(palette.link)
                            .arg(i18nc("Hyperlink", "link"))
                            .arg(palette.linkVisited)
                            .arg(i18nc("Visited hyperlink", "visited"));
                        }

                        anchors.fill: parent
                        anchors.margins: 1

                        QtControls.ItemDelegate {
                            width: parent.width
                            text: i18n("Normal text") + listPreviewColumn.demoText(model.palette)
                            activeFocusOnTab: false
                        }

                        QtControls.ItemDelegate {
                            width: parent.width
                            highlighted: true
                            text: i18n("Highlighted text") + listPreviewColumn.demoText(model.selectedPalette)
                            activeFocusOnTab: false
                        }

                        QtControls.ItemDelegate {
                            width: parent.width
                            enabled: false
                            text: i18n("Disabled text") + listPreviewColumn.demoText(model.palette)
                            activeFocusOnTab: false
                        }
                    }
                }
            }

            // Make the preview non-clickable but still reacting to hover
            MouseArea {
                anchors.fill: parent
                onClicked: delegate.clicked()
                onDoubleClicked: delegate.doubleClicked()
            }
        }

        actions: [
            Kirigami.Action {
                icon.name: "document-edit"
                tooltip: i18n("Edit Color Scheme…")
                enabled: !model.pendingDeletion
                onTriggered: kcm.editScheme(model.schemeName, root)
            },
            Kirigami.Action {
                icon.name: "edit-delete"
                tooltip: i18n("Remove Color Scheme")
                enabled: model.removable
                visible: !model.pendingDeletion
                onTriggered: model.pendingDeletion = true
            },
            Kirigami.Action {
                icon.name: "edit-undo"
                tooltip: i18n("Restore Color Scheme")
                visible: model.pendingDeletion
                onTriggered: model.pendingDeletion = false
            }
        ]
        onClicked: {
            if (root.type == ColorSchmeGrid.Type.Light) {
                kcm.model.colorSchemeLight = model.schemeName;
            } else {
                kcm.model.colorSchemeDark = model.schemeName;
            }
            view.forceActiveFocus();
        }
        onDoubleClicked: {
            kcm.save();
        }
    }
