/*
    SPDX-FileCopyrightText: 2015 Antonis Tsiapaliokas <antonis.tsiapaliokas@kde.org>
    SPDX-FileCopyrightText: 2017 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.0 as QtControls
import QtQuick.Dialogs 1.2 as QtDialogs
import org.kde.kirigami 2.3 as Kirigami
import org.kde.kcm 1.0


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
            text: root.font.family + " " + root.font.pointSize + "pt"
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

