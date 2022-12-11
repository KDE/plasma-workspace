/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import org.kde.kirigami 2.20 as Kirigami

Kirigami.SelectableLabel {
    font.family: preview.fontFamily
    font.pointSize: preview.fontSize
    font.styleName: preview.styleName
    text: backend.characters || i18nc("@info:usagetip", "No characters found.")
    wrapMode: TextEdit.WrapAnywhere
}
