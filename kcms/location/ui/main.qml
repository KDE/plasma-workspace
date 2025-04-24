/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtPositioning
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

KCM.SimpleKCM {
    id: root

    implicitHeight: Kirigami.Units.gridUnit * 45
    implicitWidth: Kirigami.Units.gridUnit * 50

    PositionSource {
        id: automaticLocationProvider
        active: !kcm.settings.manualLocation.present
    }

    ColumnLayout {
        WorldMap {
            latitude: kcm.settings.manualLocation.present ? kcm.settings.manualLocation.coordinate.latitude : automaticLocationProvider.position.coordinate.latitude
            longitude: kcm.settings.manualLocation.present ? kcm.settings.manualLocation.coordinate.longitude : automaticLocationProvider.position.coordinate.longitude
            onAccepted: (latitude, longitude) => {
                kcm.settings.manualLocation.coordinate.latitude = latitude;
                kcm.settings.manualLocation.coordinate.longitude = longitude;
                kcm.settings.manualLocation.present = true;
            }
        }

        Kirigami.FormLayout {
            ButtonGroup { id: sourceGroup }

            RadioButton {
                text: "Automatically detect location"
                checked: !kcm.settings.manualLocation.present
                onToggled: kcm.settings.manualLocation.present = false
                ButtonGroup.group: sourceGroup

                KCM.SettingStateBinding {
                    configObject: kcm.settings
                    settingName: "ManualLocation"
                }
            }

            RowLayout {
                RadioButton {
                    id: manualLocationRadioButton
                    text: "Use manual location:"
                    checked: kcm.settings.manualLocation.present
                    onToggled: kcm.settings.manualLocation.present = true
                    ButtonGroup.group: sourceGroup

                    KCM.SettingStateBinding {
                        configObject: kcm.settings
                        settingName: "ManualLocation"
                    }
                }

                Label {
                    text: "Latitude:"
                    enabled: manualLocationRadioButton.checked
                }

                DecimalSpinBox {
                    enabled: manualLocationRadioButton.checked
                    decimals: 2
                    from: -90
                    to: 90
                    value: kcm.settings.manualLocation.coordinate.latitude
                    onValueModified: kcm.settings.manualLocation.coordinate.latitude = value
                }

                Label {
                    text: "Longitude:"
                    enabled: manualLocationRadioButton.checked
                }

                DecimalSpinBox {
                    enabled: manualLocationRadioButton.checked
                    decimals: 2
                    from: -180
                    to: 180
                    value: kcm.settings.manualLocation.coordinate.longitude
                    onValueModified: kcm.settings.manualLocation.coordinate.longitude = value
                }
            }
        }
    }
}
