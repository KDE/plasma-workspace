/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.15

import org.kde.kcm 1.3
import org.kde.kirigami 2.20 as Kirigami

Kirigami.OverlaySheet {
    id: picturesSheet

    title: i18nc("@title", "Change Avatar")

    readonly property var colorPalette: [
        {"name": i18nc("@item:intable", "It's Nothing"),     "color": "transparent", "dark": false},
        {"name": i18nc("@item:intable", "Feisty Flamingo"),  "color": "#E93A9A", "dark": true},
        {"name": i18nc("@item:intable", "Dragon's Fruit"),   "color": "#E93D58", "dark": true},
        {"name": i18nc("@item:intable", "Sweet Potato"),     "color": "#E9643A", "dark": true},
        {"name": i18nc("@item:intable", "Ambient Amber"),    "color": "#EF973C", "dark": false},
        {"name": i18nc("@item:intable", "Sparkle Sunbeam"),  "color": "#E8CB2D", "dark": false},
        {"name": i18nc("@item:intable", "Lemon-Lime"),       "color": "#B6E521", "dark": false},
        {"name": i18nc("@item:intable", "Verdant Charm"),    "color": "#3DD425", "dark": false},
        {"name": i18nc("@item:intable", "Mellow Meadow"),    "color": "#00D485", "dark": false},
        {"name": i18nc("@item:intable", "Tepid Teal"),       "color": "#00D3B8", "dark": false},
        {"name": i18nc("@item:intable", "Plasma Blue"),      "color": "#3DAEE9", "dark": true},
        {"name": i18nc("@item:intable", "Pon Purple"),       "color": "#B875DC", "dark": true},
        {"name": i18nc("@item:intable", "Bajo Purple"),      "color": "#926EE4", "dark": true},
        {"name": i18nc("@item:intable", "Burnt Charcoal"),   "color": "#232629", "dark": true},
        {"name": i18nc("@item:intable", "Paper Perfection"), "color": "#EEF1F5", "dark": false},
        {"name": i18nc("@item:intable", "Cafétera Brown"),   "color": "#CB775A", "dark": false},
        {"name": i18nc("@item:intable", "Rich Hardwood"),    "color": "#6A250E", "dark": true}
    ]

    onSheetOpenChanged: if (!sheetOpen){
        destroy(Kirigami.Units.humanMoment);
    }

    contentItem: QQC2.SwipeView {
        id: stackSwitcher

        Layout.preferredWidth: usersDetailPage.width - Kirigami.Units.largeSpacing * 4

        focus: true
        interactive: false

        Keys.onEscapePressed: picturesSheet.close();

        function forceCurrentIndex(index) {
            // There is a peculiar bug in SwipeView with repeatedly switching index where it will eventually get
            // confused between SwipeView and its Container base about what the index actually should be. We
            // consequently end up on the wrong page. But only in right-to-left UI mode!
            // Forcing complete index resets fixes this problem.
            // https://bugs.kde.org/show_bug.cgi?id=439081
            setCurrentIndex(-1)
            setCurrentIndex(index)
        }

        ColumnLayout {
            id: cols

            GridLayout {
                id: picturesColumn

                rowSpacing: Kirigami.Units.smallSpacing
                columnSpacing: Kirigami.Units.smallSpacing
                columns: Math.floor((stackSwitcher.width - (Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2)) / (Kirigami.Units.gridUnit * 6 + picturesColumn.columnSpacing))

                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing

                FileDialog {
                    id: fileDialog
                    title: i18nc("@title", "Choose a picture")
                    folder: shortcuts.pictures
                    onAccepted: {
                        usersDetailPage.oldImage = usersDetailPage.user.face
                        usersDetailPage.user.face = fileDialog.fileUrl
                        usersDetailPage.overrideImage = true
                        picturesSheet.close()
                    }
                }

                QQC2.Button {
                    id: openButton
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                    Layout.preferredWidth: Layout.preferredHeight

                    contentItem: Item {
                        ColumnLayout {
                            // Centering rather than filling is desired to keep the
                            // entire layout nice and tight when the text is short
                            anchors.centerIn: parent
                            spacing: 0 // the icon should bring its own

                            Kirigami.Icon {
                                id: openIcon

                                implicitWidth: Kirigami.Units.iconSizes.huge
                                implicitHeight: Kirigami.Units.iconSizes.huge
                                source: "document-open"

                                Layout.alignment: Qt.AlignHCenter
                            }
                            QQC2.Label {
                                text: i18nc("@action:button", "Choose File…")

                                Layout.fillWidth: true
                                Layout.maximumWidth: Kirigami.Units.gridUnit * 5
                                Layout.maximumHeight: openButton.availableHeight - openIcon.height
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignBottom
                                fontSizeMode: Text.HorizontalFit
                                wrapMode: Text.Wrap
                                elide: Text.ElideRight
                            }
                        }
                    }

                    onClicked: fileDialog.open()
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
                        text: kcm.initializeString(user.displayPrimaryName)
                    }

                    onClicked: stackSwitcher.forceCurrentIndex(initialPictures.QQC2.SwipeView.index)
                }

                QQC2.Button {
                    Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                    Layout.preferredWidth: Layout.preferredHeight

                    contentItem: Kirigami.Icon {
                        anchors.centerIn: parent
                        width: Kirigami.Units.gridUnit * 4
                        height: Kirigami.Units.gridUnit * 4
                        color: "black"
                        source: "user-identity"
                    }

                    onClicked: stackSwitcher.forceCurrentIndex(iconPictures.QQC2.SwipeView.index)
                }

                Repeater {
                    model: kcm.avatarFiles
                    QQC2.Button {
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                        Layout.preferredWidth: Layout.preferredHeight

                        hoverEnabled: true

                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.text: modelData
                        QQC2.ToolTip.visible: hovered || activeFocus
                        Accessible.name: modelData

                        Image {
                            id: imgDelegate
                            visible: false
                            asynchronous: true
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
                            usersDetailPage.oldImage = usersDetailPage.user.face
                            usersDetailPage.user.face = imgDelegate.source
                            usersDetailPage.overrideImage = true
                            picturesSheet.close()
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
                columns: Math.floor((stackSwitcher.width - (Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2)) / (Kirigami.Units.gridUnit * 6))

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

                    onClicked: stackSwitcher.forceCurrentIndex(cols.QQC2.SwipeView.index)
                }

                Repeater {
                    model: picturesSheet.colorPalette
                    delegate: QQC2.Button {
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                        Layout.preferredWidth: Layout.preferredHeight

                        hoverEnabled: true

                        Accessible.name: modelData.name
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.text: modelData.name
                        QQC2.ToolTip.visible: hovered || activeFocus

                        Rectangle {
                            id: colourRectangle

                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.smallSpacing
                            color: modelData.color

                            Kirigami.Heading {
                                anchors.centerIn: parent
                                color: modelData.dark ? "white" : "black"
                                font.pixelSize: Kirigami.Units.gridUnit * 4
                                text: kcm.initializeString(user.displayPrimaryName)
                            }
                        }

                        onClicked: {
                            colourRectangle.grabToImage(function(result) {
                                const uri = kcm.plonkImageInTempfile(result.image)
                                if (uri != "") {
                                    usersDetailPage.oldImage = usersDetailPage.user.face
                                    usersDetailPage.user.face = uri
                                    usersDetailPage.overrideImage = true
                                }
                                picturesSheet.close()
                            })
                        }
                    }
                }
            }
        }

        ColumnLayout {
            id: iconPictures

            GridLayout {
                id: iconColumn

                rowSpacing: Kirigami.Units.smallSpacing
                columnSpacing: Kirigami.Units.smallSpacing
                columns: initialsColumn.columns

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

                    onClicked: stackSwitcher.forceCurrentIndex(cols.QQC2.SwipeView.index)
                }

                Repeater {
                    model: picturesSheet.colorPalette
                    delegate: QQC2.Button {
                        Layout.preferredHeight: Kirigami.Units.gridUnit * 6
                        Layout.preferredWidth: Layout.preferredHeight

                        Accessible.name: modelData.name
                        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                        QQC2.ToolTip.text: modelData.name
                        QQC2.ToolTip.visible: hovered || activeFocus

                        Rectangle {
                            id: colourRectangle

                            anchors.fill: parent
                            anchors.margins: Kirigami.Units.smallSpacing
                            color: modelData.color

                            Kirigami.Icon {
                                source: "user-identity"
                                color: modelData.dark ? "white" : "black"
                                width: Kirigami.Units.gridUnit * 4
                                height: Kirigami.Units.gridUnit * 4
                                anchors.centerIn: parent
                            }
                        }

                        onClicked: {
                            colourRectangle.grabToImage(function(result) {
                                const uri = kcm.plonkImageInTempfile(result.image)
                                if (uri != "") {
                                    usersDetailPage.oldImage = usersDetailPage.user.face
                                    usersDetailPage.user.face = uri
                                    usersDetailPage.overrideImage = true
                                }
                                picturesSheet.close()
                            })
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: open();
}
