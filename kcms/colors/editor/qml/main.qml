/*
 * Copyright 2020 Noah Davis <noahadvs@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12 as QQC2
import org.kde.kirigami 2.12 as Kirigami
import org.kde.newstuff 1.0 as KNS

// import org.kde.kcolorschemeeditor 1.0

Kirigami.AbstractApplicationWindow {
    id: root
    // TODO: use real colorscheme name
    title: "Breeze Dark"
    width: Kirigami.Units.gridUnit * 64
    height: Kirigami.Units.gridUnit * 48

    header: QQC2.ToolBar {
        Kirigami.Theme.colorSet: Kirigami.Theme.Header
        Kirigami.Theme.inherit: false
        contentItem: RowLayout {
            // For some reason, accessing the spacing property doesn't work if you use contentItem.spacing
            id: toolBarLayout
            spacing: Kirigami.Units.smallSpacing
            Kirigami.SearchField {
                id: searchField
                Layout.preferredWidth: colorList.width - header.leftPadding - toolBarLayout.spacing
    //             onAccepted: console.log("Search text is " + searchField.text)
            }
            QQC2.ToolSeparator {
                id: toolSeparator
                Layout.fillHeight: true
            }
            Kirigami.ActionToolBar {
                id: actionToolBar
                Layout.fillWidth: true
                Layout.fillHeight: true
                actions: [
                    Kirigami.Action {
                        iconName: "color-picker"
                        text: "Locate Color Role"
                        shortcut: "Ctrl+L"
                    },
                    Kirigami.Action {
                        iconName: "document-save"
                        text: "Save"
                        shortcut: "Ctrl+S"
                    },
                    Kirigami.Action {
                        iconName: "document-save-as"
                        text: "Save As..."
                        shortcut: "Ctrl+Shift+S"
                    },
                    Kirigami.Action {
                        iconName: "edit-undo"
                        text: "Undo"
                        shortcut: "Ctrl+Z"
                    },
                    Kirigami.Action {
                        iconName: "edit-redo"
                        text: "Redo"
                        shortcut: "Ctrl+Shift+Z"
                    },
                    Kirigami.Action {
                        iconName: "edit-reset"
                        text: "Reset All"
                    }
                    // Upload button from old kcolorscheme UI isn't available for QML
                ]
            }
        }
    }

    RowLayout {
        id: contentRowLayout
        anchors.fill: parent
        spacing: 0
        ColorList {
            id: colorList
            Layout.fillHeight: true
            Layout.minimumWidth: colorList.implicitWidth
            Layout.preferredWidth: Kirigami.Units.gridUnit * 16
        }
        Kirigami.Separator {
            id: sidebarSeparator
            Layout.fillHeight: true
        }
        PreviewArea {
            id: previewArea
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.minimumWidth: previewArea.implicitWidth
            Layout.minimumHeight: previewArea.implicitHeight
            Layout.preferredWidth: root.width * 0.75
        }
    }
}
