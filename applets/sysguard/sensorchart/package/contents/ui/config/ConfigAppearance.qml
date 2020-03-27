/*
 *   Copyright 2019 Marco Martin <mart@kde.org>
 *   Copyright 2019 David Edmundson <davidedmundson@kde.org>
 *   Copyright 2019 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.9
import QtQuick.Layouts 1.2
import QtQuick.Controls 2.2 as Controls

import org.kde.kirigami 2.5 as Kirigami
import org.kde.kquickcontrols 2.0
import org.kde.kconfig 1.0 // for KAuthorized
import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.ksgrd2 0.1 as KSGRD

Kirigami.FormLayout {
    id: root

    signal configurationChanged

    property alias cfg_title: titleField.text
    property string cfg_chartFace

    // config keys of the selected preset to be applied on save
    property string pendingPreset

    function saveConfig() {
        var preset = pendingPreset;
        pendingPreset = "";
        if (preset != "") {
            plasmoid.nativeInterface.currentPreset = preset;
        }
    }

    RowLayout {
        Kirigami.FormData.label: i18n("Preset:")
        Controls.ComboBox {
            id: presetCombo
            model: plasmoid.nativeInterface.availablePresetsModel

            Connections {
                target: plasmoid.nativeInterface
                onCurrentPresetChanged: {
                    for (var i = 0; i < presetCombo.count; ++i) {
                        if (presetCombo.model.pluginId(i) === plasmoid.nativeInterface.currentPreset) {
                            presetCombo.currentIndex = i;
                            return;
                        }
                    }
                }
            }
            currentIndex: {
                for (var i = 0; i < count; ++i) {
                    if (model.pluginId(i) === plasmoid.nativeInterface.currentPreset) {
                        return i;
                    }
                }
                return -1;
            }
            onActivated: {
                var presetsModel = plasmoid.nativeInterface.availablePresetsModel;
                var idx = presetsModel.index(index, 0);

                cfg_title = presetsModel.data(idx, /* DisplayRole */) || "";

                var pendingPresetConfig = presetsModel.data(idx, Qt.UserRole + 3); // FIXME proper enum
                pendingPreset = presetsModel.pluginId(index);
                if (pendingPresetConfig.chartFace) {
                    cfg_chartFace = pendingPresetConfig.chartFace;
                }

                root.configurationChanged();
            }
        }

        Controls.Button {
            icon.name: "get-hot-new-stuff"
            onClicked: plasmoid.nativeInterface.getNewPresets(this)
            visible: KAuthorized.authorize("ghns")
            Accessible.name: i18n("Get new presets")

            Controls.ToolTip {
                text: parent.Accessible.name
            }
        }

        Controls.Button {
            id: saveButton
            icon.name: "document-save"
            text: i18n("Save")
            enabled: plasmoid.nativeInterface.currentPreset.length == 0
            onClicked: plasmoid.nativeInterface.savePreset();
        }
        Controls.Button {
            icon.name: "delete"
            enabled: {
                var presetsModel = plasmoid.nativeInterface.availablePresetsModel;
                var idx = presetsModel.index(presetCombo.currentIndex, 0);
                presetCombo.currentIndex >= 0 && presetsModel.data(idx, Qt.UserRole + 4); // FIXME: proper role
            }
            onClicked: plasmoid.nativeInterface.uninstallPreset(presetCombo.model.pluginId(presetCombo.currentIndex));
        }
    }

    Kirigami.Separator {
        Kirigami.FormData.isSection: true
    }

    Controls.TextField {
        id: titleField
        Kirigami.FormData.label: i18n("Title:")
    }

    RowLayout {
        Kirigami.FormData.label: i18n("Display Style:")
        Controls.ComboBox {
            id: faceCombo
            model: plasmoid.nativeInterface.availableFacesModel
            currentIndex: {
                // TODO just make an indexOf invokable on the model?
                for (var i = 0; i < count; ++i) {
                    if (model.pluginId(i) === cfg_chartFace) {
                        return i;
                    }
                }
                return -1;
            }
            onActivated: {
                cfg_chartFace = model.pluginId(index);
            }
        }

        Controls.Button {
            icon.name: "get-hot-new-stuff"
            text: i18n("Get New Display Styles")
            visible: KAuthorized.authorize("ghns")
            onClicked: plasmoid.nativeInterface.getNewFaces(this)
        }
    }
}
