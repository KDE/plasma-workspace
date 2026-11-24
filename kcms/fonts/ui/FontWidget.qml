/*
    SPDX-FileCopyrightText: 2015 Antonis Tsiapaliokas <antonis.tsiapaliokas@kde.org>
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls
import org.kde.kirigami as Kirigami

Kirigami.FormEntry {
    id: root
    property string label
    property alias tooltipText: button.text
    property string category
    property font font

    title: root.label
    activeFocusOnTab: true

    contentItem: QtControls.TextField {
        readOnly: true
        Layout.fillWidth: true
        Kirigami.Theme.inherit: true
        text: i18nc("%1 is the name of a font type, %2 is the size in points (pt). Ex: Noto Sans 10pt", "%1 %2pt", root.font.family, root.font.pointSize)
        font: root.font
    }

    trailingItems: [
        QtControls.Button {
            id: button
            icon.name: "document-edit"
            Layout.fillHeight: true
            display: QtControls.AbstractButton.IconOnly
            text: root.tooltipText
            Kirigami.MnemonicData.enabled: false
            focus: true
            onClicked: {
                kcm.adjustFont(root.font, root.category)
            }
            QtControls.ToolTip.visible: hovered || visualFocus
            QtControls.ToolTip.text: text
            QtControls.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    ]
}

