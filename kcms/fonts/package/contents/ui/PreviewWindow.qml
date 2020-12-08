/*
    Copyright (c) 2020 Jan Blackquill <uhhadd@gmail.com>
    Copyright (c) 2020 Nate Graham <nate@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.7 as QtControls
import QtQuick.Dialogs 1.2 as QtDialogs

// For KCMShell.open()
import org.kde.kquickcontrolsaddons 2.0
import org.kde.kirigami 2.12 as Kirigami
import org.kde.kcm 1.1 as KCM

Kirigami.ShadowedRectangle {
    anchors {
        fill: parent
        margins: fontMetrics.largeSpacing * 2
    }
    radius: 3
    color: Kirigami.Theme.backgroundColor

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        Kirigami.ShadowedRectangle {
            corners {
                topLeftRadius: 3
                topRightRadius: 3
            }

            implicitHeight: titleCol.implicitHeight
            Layout.fillWidth: true
            Kirigami.Theme.colorSet: Kirigami.Theme.Header
            color: Kirigami.Theme.backgroundColor

            ColumnLayout {
                id: titleCol
                anchors.fill: parent

                RowLayout {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.margins: fontMetrics.smallSpacing

                    Kirigami.Icon {
                        source: "system-file-manager"

                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: implicitWidth
                    }
                    Kirigami.Icon {
                        source: "window-pin"

                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: implicitWidth
                    }

                    Item { Layout.fillWidth: true }

                    QtControls.Label {
                        text: i18n("Window Title")
                        font: kcm.fontsSettings.activeFont
                    }

                    Item { Layout.fillWidth: true }

                    Kirigami.Icon {
                        source: "window-minimize"

                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: implicitWidth
                    }
                    Kirigami.Icon {
                        source: "window-maximize"

                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: implicitWidth
                    }
                    Kirigami.Icon {
                        source: "window-close"

                        implicitWidth: Kirigami.Units.iconSizes.smallMedium
                        implicitHeight: implicitWidth
                    }
                }
                QtControls.ToolBar {
                    Layout.fillWidth: true
                    RowLayout {
                        anchors.fill: parent

                        QtControls.ToolButton {
                            implicitWidth: contentItem.implicitWidth + fontMetrics.smallSpacing * 3
                            implicitHeight: contentItem.implicitHeight + fontMetrics.smallSpacing * 4
                            contentItem: QtControls.Label {
                                text: i18n("Toolbar Text")
                                font: kcm.fontsSettings.toolBarFont

                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        QtControls.ToolButton {
                            implicitWidth: contentItem.implicitWidth + fontMetrics.smallSpacing * 3
                            implicitHeight: contentItem.implicitHeight + fontMetrics.smallSpacing * 4
                            contentItem: QtControls.Label {
                                text: i18n("Copy")
                                font: kcm.fontsSettings.toolBarFont

                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        QtControls.ToolButton {
                            implicitWidth: contentItem.implicitWidth + fontMetrics.smallSpacing * 3
                            implicitHeight: contentItem.implicitHeight + fontMetrics.smallSpacing * 4
                            contentItem: QtControls.Label {
                                text: i18n("Paste")
                                font: kcm.fontsSettings.toolBarFont

                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                        Item { Layout.fillWidth: true }
                        QtControls.ToolButton {
                            implicitWidth: contentItem.implicitWidth + fontMetrics.smallSpacing * 3
                            implicitHeight: contentItem.implicitHeight + fontMetrics.smallSpacing * 4
                            contentItem: QtControls.Label {
                                text: i18n("New Folder")
                                font: kcm.fontsSettings.toolBarFont

                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }
            }
        }
        QtControls.Label {
            text: i18n("General Text Sample. Lorem Ipsum Dolor Sit Amet.")
            font: kcm.fontsSettings.font

            Layout.topMargin: fontMetrics.largeSpacing
            Layout.leftMargin: fontMetrics.largeSpacing
        }
        QtControls.Label {
            text: i18n("Small Text Sample. Lorem Ipsum Dolor Sit Amet.")
            font: kcm.fontsSettings.smallestReadableFont

            Layout.topMargin: fontMetrics.largeSpacing
            Layout.leftMargin: fontMetrics.largeSpacing
        }

        Item { Layout.fillHeight: true }

        Kirigami.ShadowedRectangle {
            radius: 3

            color: Kirigami.Theme.backgroundColor
            Kirigami.Theme.colorSet: Kirigami.Theme.Complementary

            implicitWidth: fixedText.implicitWidth + fontMetrics.largeSpacing * 2
            implicitHeight: fixedText.implicitHeight + fontMetrics.largeSpacing * 2

            Layout.margins: fontMetrics.largeSpacing
            Layout.alignment: Qt.AlignRight

            QtControls.Label {
                id: fixedText
                text: i18n(`// Sample fixed width text
func main() {
    println("Lorem ipsum dolor sit amet")
}`)

                font: kcm.fontsSettings.fixed
                anchors.centerIn: parent
            }
        }
    }
}
