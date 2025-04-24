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

    actions: [
        Kirigami.Action {
            id: toggleGeolocationAction
            text: i18nc("@action: button as in, 'enable geolocation'", "Enabled")
            icon.name: "mark-location-symbolic"
            checkable: true
            checked: kcm.settings.enabled
            onToggled: kcm.settings.enabled = !kcm.settings.enabled
            displayComponent: Switch {
                action: toggleGeolocationAction
            }
        }
    ]

    PositionSource {
        id: automaticLocationProvider
        active: !kcm.settings.staticLocation.present
    }

    ColumnLayout {
        WorldMap {
            latitude: kcm.settings.staticLocation.present ? kcm.settings.staticLocation.coordinate.latitude : automaticLocationProvider.position.coordinate.latitude
            longitude: kcm.settings.staticLocation.present ? kcm.settings.staticLocation.coordinate.longitude : automaticLocationProvider.position.coordinate.longitude
            onAccepted: (latitude, longitude) => {
                kcm.settings.staticLocation.coordinate.latitude = latitude;
                kcm.settings.staticLocation.coordinate.longitude = longitude;
                kcm.settings.staticLocation.present = true;
            }
        }

        Kirigami.FormLayout {
            ButtonGroup { id: sourceGroup }

            RadioButton {
                text: "Automatically detect location"
                checked: !kcm.settings.staticLocation.present
                onToggled: kcm.settings.staticLocation.present = false
                ButtonGroup.group: sourceGroup
            }

            RowLayout {
                RadioButton {
                    id: staticLocationRadioButton
                    text: "Use manual location:"
                    checked: kcm.settings.staticLocation.present
                    onToggled: kcm.settings.staticLocation.present = true
                    ButtonGroup.group: sourceGroup
                }

                Label {
                    text: "Latitude:"
                    enabled: staticLocationRadioButton.checked
                }

                DecimalSpinBox {
                    enabled: staticLocationRadioButton.checked
                    decimals: 2
                    from: -90
                    to: 90
                    value: kcm.settings.staticLocation.coordinate.latitude
                    onValueModified: kcm.settings.staticLocation.coordinate.latitude = value
                }

                Label {
                    text: "Longitude:"
                    enabled: staticLocationRadioButton.checked
                }

                DecimalSpinBox {
                    enabled: staticLocationRadioButton.checked
                    decimals: 2
                    from: -180
                    to: 180
                    value: kcm.settings.staticLocation.coordinate.longitude
                    onValueModified: kcm.settings.staticLocation.coordinate.longitude = value
                }
            }
        }
    }
}
