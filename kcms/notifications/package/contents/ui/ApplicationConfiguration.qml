/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.9
import QtQml.Models 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QtControls

import org.kde.kirigami 2.7 as Kirigami
import org.kde.kcm 1.3 as KCM

import org.kde.notificationmanager 1.0 as NotificationManager

import org.kde.private.kcms.notifications 1.0 as Private

ColumnLayout {
    id: configColumn

    property var rootIndex
    property var behaviorSettings: rootIndex ? kcm.behaviorSettings(rootIndex) : null

    readonly property string otherAppsId: "@other"

    readonly property string appDisplayName: rootIndex ? kcm.sourcesModel.data(rootIndex, Qt.DisplayRole) || "" : ""
    readonly property string appIconName: rootIndex ? kcm.sourcesModel.data(rootIndex, Qt.DecorationRole) || "" : ""
    readonly property string desktopEntry: rootIndex ? kcm.sourcesModel.data(rootIndex, Private.SourcesModel.DesktopEntryRole) || "" : ""
    readonly property string notifyRcName: rootIndex ? kcm.sourcesModel.data(rootIndex, Private.SourcesModel.NotifyRcNameRole) || "" : ""

    readonly property bool isOtherApp: configColumn.desktopEntry === configColumn.otherAppsId

    function configureEvents(eventId) {
        kcm.configureEvents(configColumn.notifyRcName, eventId, this)
    }

    spacing: Kirigami.Units.smallSpacing

    Kirigami.FormLayout {
        id: form

        RowLayout {
            Kirigami.FormData.isSection: true
            spacing: Kirigami.Units.smallSpacing

            Kirigami.Icon {
                Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                source: configColumn.appIconName
            }

            Kirigami.Heading {
                level: 2
                text: configColumn.appDisplayName
                elide: Text.ElideRight
                textFormat: Text.PlainText
            }
        }

        QtControls.CheckBox {
            id: showPopupsCheck
            text: i18n("Show popups")
            checked: !!behaviorSettings && behaviorSettings.showPopups
            onClicked: behaviorSettings.showPopups = checked

            KCM.SettingStateBinding {
                configObject: behaviorSettings
                settingName: "ShowPopups"
            }
        }

        RowLayout { // just for indentation
            QtControls.CheckBox {
                Layout.leftMargin: mirrored ? 0 : indicator.width
                Layout.rightMargin: mirrored ? indicator.width : 0
                text: i18n("Show in do not disturb mode")
                checked: !!behaviorSettings && behaviorSettings.showPopupsInDndMode
                onClicked: behaviorSettings.showPopupsInDndMode = checked

                KCM.SettingStateBinding {
                    configObject: behaviorSettings
                    settingName: "ShowPopupsInDndMode"
                    extraEnabledConditions: showPopupsCheck.checked
                }
            }
        }

        QtControls.CheckBox {
            text: i18n("Show in history")
            checked: !!behaviorSettings && behaviorSettings.showInHistory
            onClicked: behaviorSettings.showInHistory = checked

            KCM.SettingStateBinding {
                configObject: behaviorSettings
                settingName: "ShowInHistory"
            }
        }

        QtControls.CheckBox {
            text: i18n("Show notification badges")
            checked: !!behaviorSettings && behaviorSettings.showBadges
            onClicked: behaviorSettings.showBadges = checked

            KCM.SettingStateBinding {
                configObject: behaviorSettings
                settingName: "ShowBadges"
                extraEnabledConditions: !!configColumn.desktopEntry && configColumn.desktopEntry !== configColumn.otherAppsId
            }
        }

        Kirigami.Separator {
            Kirigami.FormData.isSection: true
            visible: configureEventsButton.visible || noEventsLabel.visible
        }

        QtControls.Button {
            id: configureEventsButton
            text: i18n("Configure Events…")
            icon.name: "preferences-desktop-notification"
            visible: !!configColumn.notifyRcName
            onClicked: configColumn.configureEvents()
        }
    }

    QtControls.Label {
        id: noEventsLabel
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: form.implicitWidth
        text: i18n("This application does not support configuring notifications on a per-event basis.");
        wrapMode: Text.WordWrap
        visible: !configColumn.notifyRcName && !configColumn.isOtherApp
    }

    // compact layout
    Item {
        Layout.fillHeight: true
    }
}
