// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

import QtQuick
import org.kde.kirigami as Kirigami

QtObject {
    required property string message
    readonly property Kirigami.Action action: Kirigami.Action {
        id: action
        onTriggered: {
            Qt.openUrlExternally("https://bugs.kde.org/enter_bug.cgi?product=plasmashell&component=Session%20Management")
            Qt.quit()
        }
    }

    property alias _actionText: action.text
    property alias _actionIconName: action.icon.name
}
