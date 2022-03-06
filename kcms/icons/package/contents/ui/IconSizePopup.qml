/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QtControls
import org.kde.kirigami 2.4 as Kirigami
import org.kde.kcm 1.3 as KCM

QtControls.Popup {
    id: iconSizePopup

    width: 400
    modal: true

    onOpened: {
        // can we do this automatically with "focus: true" somewhere?
        iconTypeList.forceActiveFocus();
    }

    onVisibleChanged: {
        if (visible) {
            iconSizeSlider.sizes = kcm.availableIconSizes(iconTypeList.currentIndex);
            iconSizeSlider.updateSizes()
        }
    }

    Connections {
        target: iconTypeList
        function onCurrentIndexChanged() {
            iconSizeSlider.sizes = kcm.availableIconSizes(iconTypeList.currentIndex);
        }
    }

    RowLayout {
        anchors.fill: parent

        ColumnLayout {
            id: iconSizeColumn
            Layout.fillWidth: true

            QtControls.ItemDelegate { // purely for metrics...
                id: measureDelegate
                // without text it has no real height
                text: "For metrics only"
                visible: false
            }

            QtControls.ScrollView {
                id: iconTypeScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: iconTypeList.count * measureDelegate.height + 4
                activeFocusOnTab: false

                Component.onCompleted: iconTypeScroll.background.visible = true;

                ListView {
                    id: iconTypeList
                    activeFocusOnTab: true
                    keyNavigationEnabled: true
                    keyNavigationWraps: true
                    highlightMoveDuration: 0
                    clip: true

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

            QtControls.Slider {
                id: iconSizeSlider
                property var sizes: kcm.availableIconSizes(iconTypeList.currentIndex)

                Layout.fillWidth: true
                from: 0
                to: sizes.length - 1
                stepSize: 1.0
                snapMode: QtControls.Slider.SnapAlways

                KCM.SettingStateBinding {
                    configObject: kcm.iconsSettings
                    settingName: iconTypeList.currentItem.configKey
                    extraEnabledConditions: parent.sizes.length > 0
                }

                onMoved: {
                    kcm.iconsSettings[iconTypeList.currentItem.configKey] = iconSizeSlider.sizes[iconSizeSlider.value] || 0
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
        }

        ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: Math.round(parent.width / 2)
            Layout.maximumWidth: Math.round(parent.width / 2)

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

            QtControls.Label {
                id: iconSizeLabel
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: kcm.iconsSettings[iconTypeList.currentItem.configKey]
            }
        }
    }
}
