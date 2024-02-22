/*
    SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
    SPDX-FileCopyrightText: 2020 Carson Black <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Dialogs 6.3 as Dialogs
import QtQuick.Layouts 1.15

import org.kde.kirigami 2.20 as Kirigami

Kirigami.OverlaySheet {
    id: picturesSheet

    title: i18nc("@title", "Change Avatar")
    required property Kirigami.Page usersDetailPage

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

    component PicturesGridLayout: GridLayout {
        rowSpacing: Kirigami.Units.smallSpacing
        columns: Math.floor((stackSwitcher.implicitWidth - (Kirigami.Units.gridUnit + Kirigami.Units.largeSpacing * 2)) / (Kirigami.Units.gridUnit * 6))
        columnSpacing: Kirigami.Units.smallSpacing

        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        Layout.leftMargin: Kirigami.Units.largeSpacing
        Layout.rightMargin: Kirigami.Units.largeSpacing
    }

    component PictureButton: QQC2.Button {
        Layout.preferredHeight: Kirigami.Units.gridUnit * 6
        Layout.preferredWidth: Layout.preferredHeight
        display: QQC2.AbstractButton.IconOnly
    }

    component HomeButton: PictureButton {
        focus: !stackSwitcher.busy
        text: i18nc("@action:button", "Go Back")

        ColumnLayout {
            anchors.centerIn: parent

            Kirigami.Icon {
                width: Kirigami.Units.gridUnit * 4
                height: Kirigami.Units.gridUnit * 4
                source: "go-previous"

                Layout.alignment: Qt.AlignHCenter
            }
        }

        onClicked: stackSwitcher.pop()
    }

    component InitialsButton: PictureButton {
        text: i18nc("@action:button", "Initials")
        property alias colorRectangle: colorRectangle
        property alias color: colorRectangle.color
        property alias headingColor: heading.color

        Rectangle {
            id: colorRectangle

            color: "transparent"
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing

            Kirigami.Heading {
                id: heading
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                font.pixelSize: Kirigami.Units.gridUnit * 4
                fontSizeMode: Text.Fit
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: kcm.initializeString(user.displayPrimaryName)
                Accessible.ignored: true
            }
        }
    }

    component IconButton: PictureButton {
        property alias colorRectangle: colorRectangle
        property alias color: colorRectangle.color
        property alias iconColor: icon.color

        Rectangle {
            id: colorRectangle

            color: "transparent"
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                id: icon
                source: "user-identity"
                width: Kirigami.Units.gridUnit * 4
                height: Kirigami.Units.gridUnit * 4
                anchors.centerIn: parent
            }
        }
    }

    component MainPage: ColumnLayout {
        PicturesGridLayout {
            PictureButton {
                id: openButton

                text: i18nc("@action:button", "Choose File…")

                contentItem: Item {
                    Dialogs.FileDialog {
                        id: fileDialog
                        title: i18nc("@title", "Choose a picture")
                        onAccepted: {
                            usersDetailPage.oldImage = usersDetailPage.user.face
                            usersDetailPage.user.face = fileDialog.selectedFile
                            usersDetailPage.overrideImage = true
                            picturesSheet.close()
                        }
                    }

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
                            text: openButton.text

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

            InitialsButton {
                onClicked: stackSwitcher.push(initialsPage)
            }

            IconButton {
                text: i18nc("@action:button", "Placeholder Icon")
                iconColor: "black"
                onClicked: stackSwitcher.push(iconsPage)
            }

            Repeater {
                model: kcm.avatarFiles
                PictureButton {
                    id: delegate

                    required property string modelData

                    readonly property url source: "file:" + modelData

                    hoverEnabled: true

                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    QQC2.ToolTip.text: modelData
                    QQC2.ToolTip.visible: hovered || activeFocus
                    text: modelData

                    Kirigami.ShadowedImage {
                        id: imgDelegate
                        radius: width
                        anchors.centerIn: parent
                        width: Kirigami.Units.gridUnit * 5
                        height: Kirigami.Units.gridUnit * 5
                        sourceSize.width: width
                        sourceSize.height: height
                        asynchronous: true
                        mipmap: true
                        source: delegate.source
                    }

                    onClicked: {
                        usersDetailPage.oldImage = usersDetailPage.user.face
                        usersDetailPage.user.face = delegate.source
                        usersDetailPage.overrideImage = true
                        picturesSheet.close()
                    }
                }
            }
        }
    }

    component InitialsPage: ColumnLayout {
        PicturesGridLayout {
            HomeButton {}

            Repeater {
                model: picturesSheet.colorPalette
                delegate: InitialsButton {
                    color: modelData.color
                    headingColor: modelData.dark ? "white" : "black"
                    hoverEnabled: true

                    text: modelData.name
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                    QQC2.ToolTip.text: modelData.name
                    QQC2.ToolTip.visible: hovered || activeFocus

                    onClicked: {
                        colorRectangle.grabToImage(function(result) {
                            const path = kcm.plonkImageInTempfile(result.image)
                            if (path != "") {
                                const uri = "file://" + path
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

    component IconsPage: ColumnLayout {
        PicturesGridLayout {
            HomeButton {}

            Repeater {
                model: picturesSheet.colorPalette
                delegate: IconButton {
                    text: modelData.name
                    color: modelData.color
                    iconColor: modelData.dark ? "white" : "black"

                    Accessible.description: i18nc("@info:whatsthis", "User avatar placeholder icon")

                    onClicked: {
                        colorRectangle.grabToImage(function(result) {
                            const path = kcm.plonkImageInTempfile(result.image)
                            if (path != "") {
                                const uri = "file://" + path
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

    readonly property MainPage mainPage: MainPage {}
    readonly property InitialsPage initialsPage: InitialsPage {}
    readonly property IconsPage iconsPage: IconsPage {}

    onClosed: {
        destroy();
    }

    QQC2.StackView {
        id: stackSwitcher

        implicitWidth: usersDetailPage.width - Kirigami.Units.largeSpacing * 4
        implicitHeight: currentItem.implicitHeight

        focus: true
        initialItem: mainPage

        Keys.onEscapePressed: picturesSheet.close();
    }

    Component.onCompleted: open()
}
