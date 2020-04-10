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
import org.kde.newstuff 1.62 as NewStuff

import org.kde.ksysguard.sensors 1.0 as Sensors

Kirigami.FormLayout {
    id: root

    signal configurationChanged

    function saveConfig() {
        faceController.title = cfg_title;
        faceController.faceId = cfg_chartFace;

        var preset = pendingPreset;
        pendingPreset = "";
        if (preset != "") {
            faceController.currentPreset = preset;
        }
    }

    readonly property Sensors.SensorFaceController faceController: plasmoid.nativeInterface.faceController
    property alias cfg_title: titleField.text
    property string cfg_chartFace

    // config keys of the selected preset to be applied on save
    property string pendingPreset

    RowLayout {
        Kirigami.FormData.label: i18n("Preset:")
        Controls.ComboBox {
            id: presetCombo
            model: faceController.availablePresetsModel
            textRole: "display"

            Connections {
                target: plasmoid.nativeInterface
                onCurrentPresetChanged: {
                    for (var i = 0; i < presetCombo.count; ++i) {
                        if (presetCombo.model.pluginId(i) === faceController.currentPreset) {
                            presetCombo.currentIndex = i;
                            return;
                        }
                    }
                }
            }
            currentIndex: {
                for (var i = 0; i < count; ++i) {
                    if (model.pluginId(i) === faceController.currentPreset) {
                        return i;
                    }
                }
                return -1;
            }
            onActivated: {
                var idx = model.index(index, 0);

                cfg_title = model.data(idx, /* DisplayRole */) || "";

                var pendingPresetConfig = model.data(idx, Qt.UserRole + 3); // FIXME proper enum
                pendingPreset = model.pluginId(index);
                if (pendingPresetConfig.chartFace) {
                    cfg_chartFace = pendingPresetConfig.chartFace;
                }

                root.configurationChanged();
            }
        }

        NewStuff.Button {
            Accessible.name: i18n("Get new presets")
            configFile: "systemmonitor-presets.knsrc"
            text: ""
            onChangedEntriesChanged: faceController.availablePresetsModel.reload();
            Controls.ToolTip {
                text: parent.Accessible.name
            }
        }

        Controls.Button {
            id: saveButton
            icon.name: "document-save"
            text: i18n("Save")
            enabled: faceController.currentPreset.length == 0
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
            model: faceController.availableFacesModel
            textRole: "display"
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

        NewStuff.Button {
            text: i18n("Get New Display Styles...")
            configFile: "systemmonitor-faces.knsrc"
            onChangedEntriesChanged: faceController.availableFacesModel.reload();
        }
    }
}
