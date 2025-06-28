/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
    SPDX-FileCopyrightText: 2024 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

RowLayout {
    id: notificationHeading

    property ModelInterface modelInterface: ModelInterface {}
    property alias closeButtonTooltip: headingButtons.closeButtonTooltip

    spacing: Kirigami.Units.smallSpacing

    Kirigami.Icon {
        id: applicationIconItem
        Layout.preferredWidth: Kirigami.Units.iconSizes.small
        Layout.preferredHeight: Kirigami.Units.iconSizes.small
        source: modelInterface.applicationIconSource
        visible: valid
    }

    Kirigami.Heading {
        id: applicationNameLabel
        Layout.fillWidth: true
        level: 5
        opacity: 0.9
        textFormat: Text.PlainText
        elide: Text.ElideMiddle
        maximumLineCount: 2
        text: modelInterface.applicationName + (modelInterface.originName ? " · " + modelInterface.originName : "")
    }
    HeadingButtons {
        id: headingButtons
        modelInterface: notificationHeading.modelInterface
    }
}
