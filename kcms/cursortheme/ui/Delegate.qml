/*
    SPDX-FileCopyrightText: 2015 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

import QtQuick
import QtQuick.Window
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.kcmutils as KCM
import org.kde.private.kcm_cursortheme

KCM.GridDelegate {
    id: delegate

    text: model.display
    toolTip: model.description

    opacity: model.pendingDeletion ? 0.3 : 1

    thumbnailAvailable: true
    thumbnail: PreviewWidget {
        id: previewWidget
        anchors.fill: parent
        themeModel: kcm.cursorsModel
        currentIndex: index
        currentSize: kcm.cursorThemeSettings.cursorSize
    }

    Connections {
        target: kcm
        function onThemeApplied() {
            previewWidget.refresh();
        }
    }

    actions: [
        Kirigami.Action {
            icon.name: "edit-delete"
            tooltip: i18n("Remove Theme")
            enabled: model.isWritable
            visible: !model.pendingDeletion
            onTriggered: model.pendingDeletion = true
        },
        Kirigami.Action {
            icon.name: "edit-undo"
            tooltip: i18n("Restore Cursor Theme")
            visible: model.pendingDeletion
            onTriggered: model.pendingDeletion = false
        }
    ]

    onClicked: {
        view.forceActiveFocus();
        kcm.cursorThemeSettings.cursorTheme = kcm.cursorThemeFromIndex(index);
    }
    onDoubleClicked: {
            kcm.save();
    }
}
