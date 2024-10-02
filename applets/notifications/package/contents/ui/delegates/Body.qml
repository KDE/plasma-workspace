/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami 2.20 as Kirigami

SelectableLabel {
    id: bodyLabel

    Layout.fillWidth: true
    Layout.alignment: Qt.AlignTop

    property int maximumLineCount
    property string body

    readonly property bool truncated: maximumLineCount > 0 && bodyLabel.implicitHeight > maximumHeight
    Layout.maximumHeight: truncated ? Kirigami.Units.gridUnit * maximumLineCount : implicitHeight

    // HACK RichText does not allow to specify link color and since LineEdit
    // does not support StyledText, we have to inject some CSS to force the color,
    // cf. QTBUG-81463 and to some extent QTBUG-80354
    text: "<style>a { color: " + Kirigami.Theme.linkColor + "; }</style>" + body

    // Cannot do text !== "" because RichText adds some HTML tags even when empty
    visible: body.length > 0
    onLinkActivated: Qt.openUrlExternally(link)
}
