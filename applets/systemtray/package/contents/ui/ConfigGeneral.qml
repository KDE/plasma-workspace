/*
 *  Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *  Copyright 2014 Marco Martin <mart@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Layouts 1.0 as QtLayouts

import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kquickcontrolsaddons 2.0

Item {
    id: iconsPage

    signal configurationChanged

    width: childrenRect.width
    height: childrenRect.height
    implicitWidth: mainColumn.implicitWidth
    implicitHeight: pageColumn.implicitHeight

    property alias cfg_applicationStatusShown: applicationStatus.checked
    property alias cfg_communicationsShown: communications.checked
    property alias cfg_systemServicesShown: systemServices.checked
    property alias cfg_hardwareControlShown: hardwareControl.checked
    property alias cfg_miscellaneousShown: miscellaneous.checked
    property var cfg_extraItems: []

    SystemPalette {
        id: palette
    }

    QtLayouts.ColumnLayout {
        id: pageColumn

        PlasmaExtras.Heading {
            level: 2
            text: i18n("Categories")
            color: palette.text
        }
        Item {
            width: height
            height: units.gridUnit / 2
        }
        QtLayouts.ColumnLayout {
            spacing: units.smallSpacing * 2
            QtControls.CheckBox {
                id: applicationStatus
                text: i18n("Application Status")
            }
            QtControls.CheckBox {
                id: communications
                text: i18n("Communications")
            }
            QtControls.CheckBox {
                id: systemServices
                text: i18n("System Services")
            }
            QtControls.CheckBox {
                id: hardwareControl
                text: i18n("Hardware Control")
            }
            QtControls.CheckBox {
                id: miscellaneous
                text: i18n("Miscellaneous")
            }
        }

        Item {
            width: height
            height: units.gridUnit
        }
        PlasmaExtras.Heading {
            level: 2
            text: i18n("Extra Items")
            color: palette.text
        }
        Item {
            width: height
            height: units.gridUnit / 2
        }
        QtLayouts.ColumnLayout {
            spacing: units.smallSpacing * 2
            QtControls.CheckBox {
                id: dummyCheckbox
                visible: false
            }
            Repeater {
                model: plasmoid.nativeInterface.availablePlasmoids
                delegate: QtControls.CheckBox {
                    QtLayouts.Layout.minimumWidth: childrenRect.width
                    checked: cfg_extraItems.indexOf(plugin) != -1
                    onCheckedChanged: {
                        var index = cfg_extraItems.indexOf(plugin);
                        if (checked) {
                            if (index == -1) {
                                cfg_extraItems.push(plugin);
                            }
                        } else {
                            if (index > -1) {
                                cfg_extraItems.splice(index, 1);
                            }
                        }
                        configurationChanged() // qml cannot detect changes inside an Array
                    }
                    QtLayouts.RowLayout {
                        anchors.verticalCenter: parent.verticalCenter
                        x: dummyCheckbox.width
                        QIconItem {
                            icon: decoration
                            width: units.iconSizes.small
                            height: width
                        }
                        QtControls.Label {
                            text: display
                        }
                    }
                }
            }
        }
    }
}
