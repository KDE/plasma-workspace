/*
 * Copyright 2014 Marco Martin <mart@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.milou 0.1 as Milou

ColumnLayout {
    id: root
    property string query
    property string runner

    onQueryChanged: {
        queryField.text = query;
    }

    Connections {
        target: runnerWindow
        onVisibleChanged: {
            if (runnerWindow.visible) {
                queryField.forceActiveFocus();
            } else {
                root.query = "";
            }
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignTop
        PlasmaComponents.ToolButton {
            iconSource: "configure"
            onClicked: {
                runnerWindow.visible = false
                runnerWindow.displayConfiguration()
            }
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Configure")
            Accessible.description: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Configure Search Plugins")
        }
        PlasmaComponents.TextField {
            id: queryField
            clearButtonShown: true
            Layout.minimumWidth: units.gridUnit * 25
            onTextChanged: {
                root.query = queryField.text
            }
            Keys.onEscapePressed: {
                runnerWindow.visible = false
            }
            Keys.forwardTo: results
        }
        PlasmaComponents.ToolButton {
            iconSource: "window-close"
            onClicked: runnerWindow.visible = false
            Accessible.name: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Close")
            Accessible.description: i18nd("plasma_lookandfeel_org.kde.lookandfeel", "Close Search")
        }
    }

    PlasmaExtras.ScrollArea {
        Layout.alignment: Qt.AlignTop
        visible: results.count > 0
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(Screen.height, results.contentHeight)

        Milou.ResultsView {
            id: results
            queryString: root.query
            runner: root.runner

            onActivated: {
                runnerWindow.visible = false
            }

            onUpdateQueryString: {
                queryField.text = text
                queryField.cursorPosition = cursorPosition
            }
        }
    }
}
