/*
    SPDX-FileCopyrightText: 2020 Tobias Fella <fella@posteo.de>
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kitemmodels as KItemModels

QQC2.ComboBox {
    id: comboBox

    property var component

    model: component.applications
    textRole: "name"
    currentIndex: component.index

    Kirigami.StyleHints.iconName: model.data(model.index(currentIndex, 0), model.KItemModels.KRoleNames.role("icon")) ?? ""

    onActivated: component.select(currentIndex)

    delegate: QQC2.ItemDelegate {
        width: ListView.view.width
        text: model.name
        highlighted: comboBox.highlightedIndex == index
        icon.name: model.icon
    }
}
