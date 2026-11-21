/*
    SPDX-FileCopyrightText: 2013 David Edmundson <davidedmundso*n@kde.org>
    SPDX-FileCopyrightText: 2014 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Mikel Johnson <mikel5764@gmail.com>
    SPDX-FileCopyrightText: 2022 Nate Graham <nate@kde.org>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.iconthemes as KIconThemes
import org.kde.ksvg as KSvg
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

import "code/Tools.js" as Tools

QQC2.Button {
    id: iconButton

    required property string iconName
    required property string widgetName
    required property bool hasLabelText

    Kirigami.FormData.label: i18nc("@label prefix for icon-only button", "Icon:")

    implicitWidth: previewFrame.width + Kirigami.Units.smallSpacing * 2
    implicitHeight: previewFrame.height + Kirigami.Units.smallSpacing * 2
    hoverEnabled: true

    // Just to provide some visual feedback when dragging;
    // cannot have checked without checkable enabled
    checkable: true
    checked: dropArea.containsAcceptableDrag

    Accessible.name: i18nc("@action:button %1 is widget name", "Change %1's icon", iconButton.widgetName)
    Accessible.description: i18nc("@info:whatsthis", "Current icon is %1. Click to open menu to change the current icon or reset to the default icon.", iconButton.iconName)
    Accessible.role: Accessible.ButtonMenu

    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    QQC2.ToolTip.text: i18nc("@info:tooltip", "Icon name is \"%1\"", iconButton.iconName)
    QQC2.ToolTip.visible: (hovered || visualFocus) && iconButton.iconName.length > 0

    KIconThemes.IconDialog {
        id: iconDialog
        onAccepted: {
            iconButton.iconName = iconName || Tools.defaultIconName;
        }
    }

    onPressed: iconMenu.opened ? iconMenu.close() : iconMenu.open()

    DropArea {
        id: dropArea

        property bool containsAcceptableDrag: false

        anchors.fill: parent

        onEntered: event => {
            // Cannot use string operations (e.g. indexOf()) on "url" basic type.
            const urlString = event.urls[0].toString();

            // This list is also hardcoded in KIconDialog.
            const extensions = [".png", ".xpm", ".svg", ".svgz"];
            containsAcceptableDrag = urlString.startsWith("file:///")
            && extensions.some(extension => urlString.endsWith(extension));

            if (!containsAcceptableDrag) {
                event.accepted = false;
            }
        }
        onExited: containsAcceptableDrag = false

        onDropped: event => {
            if (containsAcceptableDrag) {
                // Strip file:// prefix, we already verified in onEntered that we have only local URLs.
                iconButton.iconName = event.urls[0].toString().substr("file://".length);
            }
            containsAcceptableDrag = false;
        }
    }

    KSvg.FrameSvgItem {
        id: previewFrame
        anchors.centerIn: parent
        imagePath: Plasmoid.formFactor === PlasmaCore.Types.Vertical || Plasmoid.formFactor === PlasmaCore.Types.Horizontal
            ? "widgets/panel-background" : "widgets/background"
        width: Kirigami.Units.iconSizes.large + fixedMargins.left + fixedMargins.right
        height: Kirigami.Units.iconSizes.large + fixedMargins.top + fixedMargins.bottom

        Kirigami.Icon {
            anchors.centerIn: parent
            width: Kirigami.Units.iconSizes.large
            height: width
            source: Tools.iconOrDefault(Plasmoid.formFactor, iconButton.iconName)
        }
    }

    QQC2.Menu {
        id: iconMenu

        // Appear below the button
        y: parent.height

        QQC2.MenuItem {
            text: i18nc("@item:inmenu Open icon chooser dialog", "Choose…")
            icon.name: "document-open-folder"
            Accessible.description: i18nc("@info:whatsthis", "Choose an icon for %1", iconButton.widgetName)
            onClicked: iconDialog.open()
        }
        QQC2.MenuItem {
            text: i18nc("@item:inmenu Reset icon to default", "Reset to default icon")
            icon.name: "edit-clear"
            enabled: iconButton.iconName !== Tools.defaultIconName
            onClicked: iconButton.iconName = Tools.defaultIconName
        }
        QQC2.MenuItem {
            text: i18nc("@action:inmenu", "Remove icon")
            icon.name: "delete"
            enabled: iconButton.iconName !== "" && iconButton.hasLabelText && Plasmoid.formFactor !== PlasmaCore.Types.Vertical
            onClicked: iconButton.iconName = ""
        }
    }
}
