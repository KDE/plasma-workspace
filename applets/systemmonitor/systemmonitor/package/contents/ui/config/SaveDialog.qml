/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.3
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.2
import QtQuick.Dialogs 1.2
import org.kde.kirigami 2.8 as Kirigami
import org.kde.plasma.plasmoid 2.0

Dialog {
    id: dialog
    property alias pluginName: pluginNameField.text
    property alias comment: commentField.text
    property alias author: authorField.text
    property alias email: emailField.text
    property alias license: licenseField.editText
    property alias website: websiteField.text

    property bool canEdit: false

    width: 500
    title: i18n("Save Preset %1", plasmoid.configuration.title)

    onVisibleChanged: {
        pluginNameField.focus = true
    }

    contentItem: Rectangle {
        implicitWidth:  layout.Layout.minimumWidth + units.smallSpacing*2
        implicitHeight: layout.Layout.minimumHeight + units.smallSpacing*2

        Keys.onPressed: {
            if (event.key == Qt.Key_Enter || event.key == Qt.Key_Return) {
                dialog.accept();
            } else if (event.key == Qt.Key_Escape) {
                dialog.reject();
            }
        }

        color: Kirigami.Theme.backgroundColor

        ColumnLayout {
            id: layout
            anchors {
                fill: parent
                margins: units.smallSpacing
            }
            Kirigami.InlineMessage { //standard comp
                id: errorMessage
                type: Kirigami.MessageType.Error
                text: ""
                Layout.fillWidth: true
                visible: text.length > 0
            }
            Kirigami.FormLayout {
                Layout.fillWidth: true

                TextField {
                    id: pluginNameField
                    Layout.fillWidth: true
                    Kirigami.FormData.label: i18n("Theme Plugin Name:")
                    onTextChanged: {
                        var presetsModel = plasmoid.nativeInterface.availablePresetsModel;
                        for (var i = 0; i < plasmoid.nativeInterface.availablePresetsModel.rowCount(); ++i) {
                            var idx = presetsModel.index(i, 0);
                            if (pluginNameField.text == presetsModel.data(idx, Qt.UserRole + 1)) { // FIXME proper enum
                                dialog.canEdit = false;
                                errorMessage.text = i18n("This theme plugin name already exists");
                                return;
                            }
                        }
                        errorMessage.text = "";
                        dialog.canEdit = true;
                    }
                }
                TextField {
                    id: commentField
                    Kirigami.FormData.label: i18n("Comment:")
                    Layout.fillWidth: true
                }
                TextField {
                    id: authorField
                    Kirigami.FormData.label: i18n("Author:")
                    Layout.fillWidth: true
                }
                TextField {
                    id: emailField
                    Kirigami.FormData.label: i18n("Email:")
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: licenseField
                    Kirigami.FormData.label: i18n("License:")
                    Layout.fillWidth: true
                    editable: true
                    editText: "LGPL 2.1+"
                    model: ["LGPL 2.1+", "GPL 2+", "GPL 3+", "LGPL 3+", "BSD"]
                }
                TextField {
                    id: websiteField
                    Kirigami.FormData.label: i18n("Website:")
                    Layout.fillWidth: true
                }
            }
            Item {
                Layout.fillHeight: true
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                Button {
                    text: i18n("OK")
                    onClicked: dialog.accept()
                    enabled: canEdit && authorField.text && emailField.text && websiteField.text
                }
                Button {
                    text: i18n("Cancel")
                    onClicked: dialog.reject()
                }
            }
        }
    }

    onAccepted: {
        plasmoid.nativeInterface.createNewPreset(pluginNameField.text, commentField.text, authorField.text, emailField.text, licenseField.editText, websiteField.text);
    }
}
