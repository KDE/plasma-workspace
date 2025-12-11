/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import QtQuick.Dialogs as QtDialogs
import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami
import org.kde.private.kcms.lookandfeel as Private

KCM.SimpleKCM {
    title: i18nc("@title:window", "Save Current Theme")

    actions: [
        Kirigami.Action {
            enabled: !!nameTextField.text && nameTextField.acceptableInput
            icon.name: "checkmark"
            text: i18nc("@action:button", "Confirm")
            onTriggered: {
                kcm.saveCurrentTheme(nameTextField.text, previewImage.source);
                kcm.pop();
            }
        }
    ]

    Kirigami.FormLayout {
        ColumnLayout {
            QtControls.Label {
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.gridUnit

                text: i18nc("@info", "The current Plasma settings will be saved in a new global theme.")
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }

            QtControls.AbstractButton {
                id: button
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Kirigami.Units.gridUnit * 20
                Layout.preferredHeight: Kirigami.Units.gridUnit * 10

                contentItem: Kirigami.ShadowedRectangle {
                    id: delegate
                    Kirigami.Theme.inherit: false
                    Kirigami.Theme.colorSet: Kirigami.Theme.View
                    implicitWidth: implicitHeight * 1.6
                    implicitHeight: Kirigami.Units.gridUnit * 5
                    color: {
                        if (button.pressed) {
                            return Kirigami.Theme.highlightColor;
                        } else if (button.hovered || button.visualFocus) {
                            return Qt.rgba(Kirigami.Theme.highlightColor.r, Kirigami.Theme.highlightColor.g, Kirigami.Theme.highlightColor.b, 0.5);
                        } else {
                            return Kirigami.Theme.backgroundColor;
                        }
                    }
                    radius: Kirigami.Units.cornerRadius
                    shadow.xOffset: 0
                    shadow.yOffset: 2
                    shadow.size: 10
                    shadow.color: Qt.rgba(0, 0, 0, 0.3)

                    Image {
                        id: previewImage
                        anchors.fill: parent
                        anchors.margins: Kirigami.Units.smallSpacing
                        asynchronous: true
                        fillMode: Image.PreserveAspectCrop
                        source: currentLookAndFeelInformation.preview
                    }
                }

                onClicked: previewDialog.open()
            }

            QtControls.Button {
                Layout.alignment: Qt.AlignHCenter
                text: i18nc("@action:button Configure day-night cycle times", "Take a screenshot…")
                onClicked: screenshotMaker.take()
            }
        }

        Item {
            height: Kirigami.Units.largeSpacing
        }

        QtControls.TextField {
            id: nameTextField
            Kirigami.FormData.label: i18n("Name:")
            focus: true
            placeholderText: i18nc("@info:placeholder", "Type in name…")
            validator: Private.LookAndFeelNameValidator {
                model: kcm.model
            }
        }

        Kirigami.InlineMessage {
            implicitWidth: nameTextField.width
            type: Kirigami.MessageType.Warning
            visible: !nameTextField.acceptableInput
            text: i18nc("@info", "A global theme with such a name or id already exists.")
        }
    }

    Private.LookAndFeelInformation {
        id: currentLookAndFeelInformation
        model: kcm.model
        packageId: kcm.settings.lookAndFeelPackage
    }

    Private.ScreenshotMaker {
        id: screenshotMaker
        onAccepted: fileUrl => previewImage.source = fileUrl
    }

    QtDialogs.FileDialog {
        id: previewDialog
        nameFilters: [i18nc("@option image file formats; do not translate *.png *.jpg *.jpeg *.bmp", "Image Files (*.png *.jpg *.jpeg *.bmp)")]
        title: i18nc("@title", "Choose a preview image")
        onAccepted: previewImage.source = selectedFile
    }
}
