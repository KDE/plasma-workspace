/*
 *  SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2

QQC2.RadioButton {
    id: controlRoot

    property alias subtitle: subtitleItem.text

    readonly property Item __layout: ColumnLayout {
        spacing: 0
        Item {
            id: contentItemPlaceholder
            Layout.fillWidth: true
        }
        QQC2.Label {
            id: subtitleItem
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            opacity: 0.7
            Layout.fillWidth: true
        }
    }

    Component.onCompleted: {
        const item = this.contentItem;
        this.contentItem = __layout;
        item.parent = contentItemPlaceholder;
        item.visible = true;
        contentItemPlaceholder.implicitWidth = Qt.binding(() => item.implicitWidth);
        contentItemPlaceholder.implicitHeight = Qt.binding(() => item.implicitHeight);
        indicator.y = Qt.binding(() => controlRoot.topPadding);
        if (item instanceof Text) {
            item.wrapMode = Text.Wrap;
            item.elide = Text.ElideNone;
            subtitleItem.leftPadding = item.leftPadding;
            subtitleItem.rightPadding = item.rightPadding;
        }
    }
}
