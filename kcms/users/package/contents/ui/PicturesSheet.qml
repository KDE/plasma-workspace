/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.6
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.12
import QtQuick.Controls 2.5 as QQC2
import QtQuick.Dialogs 1.3

import org.kde.kcm 1.2
import org.kde.kirigami 2.13 as Kirigami

Kirigami.OverlaySheet {
    id: picturesSheet

    title: i18n("Change Avatar")

    property var fileDialog: FileDialog {
        title: i18n("Choose a picture")
        folder: shortcuts.pictures
        onAccepted: {
            picturesSheet.close()
            usersDetailPage.oldImage = usersDetailPage.user.face
            usersDetailPage.user.face = fileDialog.fileUrl
            usersDetailPage.overrideImage = true
        }
    }

    onSheetOpenChanged: {
        if (sheetOpen) {
            contentLoader.active = true
            contentLoader.item.forceActiveFocus()
        }
    }

    Loader {
        id: contentLoader
        active: false
        sourceComponent: Component {
            QQC2.SwipeView {
                id: stackSwitcher
                interactive: false

                Layout.preferredWidth: usersDetailPage.width - (Kirigami.Units.largeSpacing*4)
                Keys.onEscapePressed: {
                    picturesSheet.close()
                    event.accepted = true
                }
                ColumnLayout {
                    id: cols
                    GridLayout {
                        id: picturesColumn

                        rowSpacing: Kirigami.Units.smallSpacing
                        columnSpacing: Kirigami.Units.smallSpacing
                        columns: {
                            // subtract gridunit from stackswticher width to roughly compensate for slight overlap on tightly fit grids
                            return Math.floor((stackSwitcher.width - Kirigami.Units.gridUnit) / ((Kirigami.Units.gridUnit * 6) + picturesColumn.columnSpacing))
                        }

                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.leftMargin: Kirigami.Units.largeSpacing
                        Layout.rightMargin: Kirigami.Units.largeSpacing

                        QQC2.Button {
                            Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                            Layout.preferredWidth: Layout.preferredHeight

                            ColumnLayout {
                                anchors.centerIn: parent

                                Kirigami.Icon {
                                    implicitWidth: Kirigami.Units.gridUnit * 4
                                    implicitHeight: Kirigami.Units.gridUnit * 4
                                    source: "document-open"

                                    Layout.alignment: Qt.AlignHCenter
                                }
                                QQC2.Label {
                                    text: i18n("Choose File…")

                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignBottom
                                    fontSizeMode: Text.HorizontalFit
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                    Layout.maximumWidth: Kirigami.Units.gridUnit * 5
                                }
                            }

                            onClicked: picturesSheet.fileDialog.open()
                        }

                        QQC2.Button {
                            Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                            Layout.preferredWidth: Layout.preferredHeight

                            Kirigami.Heading {
                                anchors.fill: parent
                                anchors.margins: Kirigami.Units.smallSpacing
                                font.pixelSize: Kirigami.Units.gridUnit * 4
                                fontSizeMode: Text.Fit
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                text: kcm.initializeString(user.realName)
                            }

                            onClicked: stackSwitcher.currentIndex = 1
                        }

                        Repeater {
                            model: kcm.avatarFiles
                            QQC2.Button {
                                Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                                Layout.preferredWidth: Layout.preferredHeight

                                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                QQC2.ToolTip.text: modelData
                                Accessible.name: modelData

                                Image {
                                    id: imgDelegate
                                    visible: false
                                    smooth: true
                                    mipmap: true
                                    sourceSize.width: Kirigami.Units.gridUnit * 5
                                    sourceSize.height: Kirigami.Units.gridUnit * 5
                                    source: modelData

                                    Accessible.ignored: true
                                }

                                Kirigami.ShadowedTexture {
                                    radius: width / 2
                                    anchors.centerIn: parent
                                    width: Kirigami.Units.gridUnit * 5
                                    height: Kirigami.Units.gridUnit * 5

                                    source: imgDelegate
                                }

                                onClicked: {
                                    picturesSheet.close()
                                    usersDetailPage.oldImage = usersDetailPage.user.face
                                    usersDetailPage.user.face = imgDelegate.source
                                    usersDetailPage.overrideImage = true
                                }
                            }
                        }
                    }
                }
                ColumnLayout {
                    id: initialPictures
                    GridLayout {
                        id: initialsColumn

                        rowSpacing: Kirigami.Units.smallSpacing
                        columnSpacing: Kirigami.Units.smallSpacing
                        columns: {
                            return Math.floor((stackSwitcher.width) / (Kirigami.Units.gridUnit * 6))
                        }

                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignHCenter
                        Layout.leftMargin: Kirigami.Units.largeSpacing
                        Layout.rightMargin: Kirigami.Units.largeSpacing

                        QQC2.Button {
                            Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                            Layout.preferredWidth: Layout.preferredHeight

                            ColumnLayout {
                                anchors.centerIn: parent

                                Kirigami.Icon {
                                    width: Kirigami.Units.gridUnit * 4
                                    height: Kirigami.Units.gridUnit * 4
                                    source: "go-previous"

                                    Layout.alignment: Qt.AlignHCenter
                                }
                            }

                            onClicked: stackSwitcher.currentIndex = 0
                        }

                        Repeater {
                            model: [
                                {"name": i18n("Feisty Flamingo"),  "color": "#E93A9A", "dark": true},
                                {"name": i18n("Dragon's Fruit"),   "color": "#E93D58", "dark": true},
                                {"name": i18n("Sweet Potato"),     "color": "#E9643A", "dark": true},
                                {"name": i18n("Ambient Amber"),    "color": "#EF973C", "dark": false},
                                {"name": i18n("Sparkle Sunbeam"),  "color": "#E8CB2D", "dark": false},
                                {"name": i18n("Lemon-Lime"),       "color": "#B6E521", "dark": false},
                                {"name": i18n("Verdant Charm"),    "color": "#3DD425", "dark": false},
                                {"name": i18n("Mellow Meadow"),    "color": "#00D485", "dark": false},
                                {"name": i18n("Tepid Teal"),       "color": "#00D3B8", "dark": false},
                                {"name": i18n("Plasma Blue"),      "color": "#3DAEE9", "dark": true},
                                {"name": i18n("Pon Purple"),       "color": "#B875DC", "dark": true},
                                {"name": i18n("Bajo Purple"),      "color": "#926EE4", "dark": true},
                                {"name": i18n("Burnt Charcoal"),   "color": "#232629", "dark": true},
                                {"name": i18n("Paper Perfection"), "color": "#EEF1F5", "dark": false},
                                {"name": i18n("Cafétera Brown"),   "color": "#CB775A", "dark": false},
                                {"name": i18n("Rich Hardwood"),    "color": "#6A250E", "dark": true}
                            ]
                            delegate: QQC2.Button {
                                Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                                Layout.preferredWidth: Layout.preferredHeight

                                Rectangle {
                                    id: colourRectangle

                                    anchors.fill: parent
                                    anchors.margins: Kirigami.Units.smallSpacing
                                    color: modelData.color

                                    Rectangle {
                                        anchors.fill: parent
                                        gradient: Gradient {
                                            GradientStop { position: 0.0; color: "transparent" }
                                            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.3) }
                                        }
                                    }

                                    Kirigami.Heading {
                                        anchors.centerIn: parent
                                        color: modelData.dark ? "white" : "black"
                                        font.pixelSize: Kirigami.Units.gridUnit * 4
                                        text: kcm.initializeString(user.realName)
                                    }
                                }

                                onClicked: {
                                    colourRectangle.grabToImage(function(result) {
                                        picturesSheet.close()
                                        let uri = kcm.plonkImageInTempfile(result.image)
                                        if (uri != "") {
                                            usersDetailPage.oldImage = usersDetailPage.user.face
                                            usersDetailPage.user.face = uri
                                            usersDetailPage.overrideImage = true
                                        }
                                    })
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
