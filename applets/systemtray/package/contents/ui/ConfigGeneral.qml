

/***************************************************************************
 *   Copyright (C) 2020 Konrad Materka <materka@gmail.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/
import QtQuick 2.14
import QtQuick.Controls 2.14 as QQC2
import org.kde.kirigami 2.13 as Kirigami

Kirigami.FormLayout {

    property alias cfg_iconSize: iconSizeComboBox.currentIndex

    QQC2.ComboBox {
        id: iconSizeComboBox

        Kirigami.FormData.label: i18n("Maximum icon size:")

        model: [i18n("Very Small"), i18n("Small"), i18n("Medium"), i18n("Large"), i18n("Huge"), i18n("Enormous")]
    }

}
