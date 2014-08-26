/***************************************************************************
 *   Copyright (C) 2014 by Daniel Vrátil <dvratil@redhat.com>              *
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

import QtQuick 2.1

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents

import org.kde.kscreenlocker 1.0

PlasmaComponents.ToolButton {
    id: kbLayoutButton
    flat: false
    checkable: false
    activeFocusOnTab: true

    Accessible.name: i18ndc("plasma_lookandfeel_org.kde.lookandfeel", "Button to change keyboard layout", "Switch layout")

    onClicked: {
        layout.nextLayout();
    }

    visible: layout.layouts.length > 1


    KeyboardLayout {
          id: layout;

          function nextLayout() {
              var layouts = layout.layouts;
              var index = layouts.indexOf(layout.currentLayout);
              if (index == (layouts.length - 1)) {
                  index = -1
              }

              layout.currentLayout = layouts[index + 1];
          }
    }

    // Use Label so that we don't set ToolButton's text and icon, which makes the
    // button tiny and the entire thing looks much cooler
    PlasmaComponents.Label {
          id: sizeHelper

          anchors.fill: parent

          verticalAlignment: Qt.AlignVCenter
          horizontalAlignment: Qt.AlignHCenter
          text: layout.currentLayout
    }
}
