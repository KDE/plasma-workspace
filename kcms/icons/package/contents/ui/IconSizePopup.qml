/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kcm 1.3 as KCM

RowLayout {
    id: iconSizePopup

    signal opened
    onOpened: {
        iconSizeSlider.sizes = kcm.availableIconSizes(iconTypeList.currentIndex);
        iconSizeSlider.updateSizes()
        // can we do this automatically with "focus: true" somewhere?
        iconTypeList.forceActiveFocus();
    }

    Connections {
        target: iconTypeList
        function onCurrentIndexChanged() {
            iconSizeSlider.sizes = kcm.availableIconSizes(iconTypeList.currentIndex);
        }
    }

    QtControls.ScrollView {
        id: iconTypeScroll
        Layout.minimumWidth: Math.round(rightPanel.implicitWidth / 2)
        Layout.minimumHeight: rightPanel.implicitHeight
        Layout.fillHeight: true
        activeFocusOnTab: false

        Component.onCompleted: iconTypeScroll.background.visible = true;

        ListView {
            id: iconTypeList
            activeFocusOnTab: true
            keyNavigationEnabled: true
            keyNavigationWraps: true
            highlightMoveDuration: 0

            model: kcm.iconSizeCategoryModel
            currentIndex: 0 // Initialize with the first item

            Keys.onLeftPressed: {
                LayoutMirroring.enabled ? iconSizeSlider.increase() : iconSizeSlider.decrease()
                iconSizeSlider.moved();
            }
            Keys.onRightPressed: {
                LayoutMirroring.enabled ? iconSizeSlider.decrease() : iconSizeSlider.increase()
                iconSizeSlider.moved();
            }

            delegate: QtControls.ItemDelegate {
                width: ListView.view.width
                highlighted: ListView.isCurrentItem
                text: model.display
                readonly property string configKey: model.configKey
                onClicked: {
                    ListView.view.currentIndex = index;
                    ListView.view.forceActiveFocus();
                }
            }
        }
    }

    ColumnLayout {
        id: rightPanel
        Layout.fillHeight: true
        Layout.minimumWidth: Kirigami.Units.iconSizes.enormous * 2
        Layout.minimumHeight: Kirigami.Units.iconSizes.enormous * 2

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Kirigami.Icon {
                anchors.centerIn: parent
                width: kcm.iconsSettings[iconTypeList.currentItem.configKey]
                height: width
                source: "folder"
            }
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QtControls.Label {
                text: i18nc("@label:slider", "Size:")
            }

            QtControls.Slider {
                id: iconSizeSlider
                property var sizes: kcm.availableIconSizes(iconTypeList.currentIndex)

                Layout.fillWidth: true
                from: 0
                to: sizes.length - 1
                stepSize: 1.0
                snapMode: QtControls.Slider.SnapAlways
                enabled: sizes.length > 0 && !kcm.iconsSettings.isImmutable(iconTypeList.currentItem.configKey)

                onMoved: {
                    kcm.iconsSettings[iconTypeList.currentItem.configKey] = iconSizeSlider.sizes[iconSizeSlider.value] || 0
                }

                KCM.SettingStateBinding {
                    configObject: kcm.iconsSettings
                    settingName: iconTypeList.currentItem.configKey
                    extraEnabledConditions: parent.sizes.length > 0
                }

                function updateSizes() {
                    // since the icon sizes are queried using invokables, always force an update when opening
                    // in case the user clicked Default or something
                    value = Qt.binding(function() {
                        var iconSize = kcm.iconsSettings[iconTypeList.currentItem.configKey]

                        // I have no idea what this code does but it works and is just copied from the old KCM
                        var index = -1;
                        var delta = 1000;
                        for (var i = 0, length = sizes.length; i < length; ++i) {
                            var dw = Math.abs(iconSize - sizes[i]);
                            if (dw < delta) {
                                delta = dw;
                                index = i;
                            }
                        }

                        return index;
                    });
                }
            }

            QtControls.Label {
                Layout.minimumWidth: Kirigami.Units.gridUnit * 2
                horizontalAlignment: Text.AlignHCenter
                text: kcm.iconsSettings[iconTypeList.currentItem.configKey]
            }
        }
    }
}
