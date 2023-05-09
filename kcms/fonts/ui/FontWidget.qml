/*
    SPDX-FileCopyrightText: 2015 Antonis Tsiapaliokas <antonis.tsiapaliokas@kde.org>
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0 as QtControls
import org.kde.kirigami 2.3 as Kirigami
import org.kde.kcmutils

FocusScope {
    id: root
    property string label
    property alias tooltipText: tooltip.text
    property string category
    property font font
    Kirigami.FormData.label: root.label
    activeFocusOnTab: true

    implicitWidth: layout.implicitWidth
    implicitHeight: layout.implicitHeight

    RowLayout {
        id: layout

        QtControls.TextField {
            readOnly: true
            Kirigami.Theme.inherit: true
            text: i18nc("%1 is the name of a font type, %2 is the size in points (pt). Ex: Noto Sans 10pt", "%1 %2pt", root.font.family, root.font.pointSize)
            font: root.font
        }

        QtControls.Button {
            icon.name: "document-edit"
            Layout.fillHeight: true
            Kirigami.MnemonicData.enabled: false
            focus: true
            onClicked: {
                fontDialog.adjustAllFonts = false
                kcm.adjustFont(root.font, root.category)
            }
            QtControls.ToolTip {
                id: tooltip
            }
        }
    }
}

