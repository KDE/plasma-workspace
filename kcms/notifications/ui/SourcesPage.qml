/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QtControls

import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCM

import org.kde.private.kcms.notifications as Private

KCM.AbstractKCM {
    id: sourcesPage
    // Use app title when when only configuring events.
    title: showOnlyEventsConfig ? rootIndex.model.data(rootIndex, Qt.DisplayRole) : i18n("Application Settings")

    property alias rootIndex: appConfiguration.rootIndex
    property bool showOnlyEventsConfig: false

    framedView: false

    Component.onCompleted: {
        var idx = kcm.sourcesModel.persistentIndexForDesktopEntry(kcm.initialDesktopEntry);
        if (!idx.valid) {
            idx = kcm.sourcesModel.persistentIndexForNotifyRcName(kcm.initialNotifyRcName);
        }
        if (idx.valid) {
            appConfiguration.rootIndex = idx;
        }

        // In Component.onCompleted we might not be assigned a window yet
        // which we need to make the events config dialog transient to it
        Qt.callLater(function() {
            if (kcm.initialEventId && kcm.initialNotifyRcName) {
                appConfiguration.configureEvents(kcm.initialEventId);
            }

            kcm.initialDesktopEntry = "";
            kcm.initialNotifyRcName = "";
            kcm.initialEventId = "";
        });
    }

    Binding {
        target: kcm.filteredModel
        property: "query"
        value: searchField.text
        restoreMode: Binding.RestoreBinding
    }

    header: ColumnLayout {
        visible: !sourcesPage.showOnlyEventsConfig

        Kirigami.SearchField {
            id: searchField

            Layout.fillWidth: true
        }
    }

    RowLayout {
        id: rootRow

        anchors.fill: parent

        spacing: 0

        QtControls.ScrollView {
            id: sourcesScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: Kirigami.Units.gridUnit * 12
            Layout.preferredWidth: Math.round(rootRow.width / 3)
            visible: !sourcesPage.showOnlyEventsConfig

            background: null

            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false

            contentItem: ListView {
                id: sourcesList
                clip: true
                focus: true
                activeFocusOnTab: true

                keyNavigationEnabled: true
                keyNavigationWraps: true
                highlightMoveDuration: 0

                model: kcm.filteredModel
                currentIndex: -1

                section {
                    criteria: ViewSection.FullString
                    property: "sourceType"
                    delegate: Kirigami.ListSectionHeader {
                        id: sourceSection
                        width: sourcesList.width
                        label: {
                            switch (Number(section)) {
                                case Private.SourcesModel.ApplicationType: return i18n("Applications");
                                case Private.SourcesModel.ServiceType: return i18n("System Services");
                            }
                        }
                    }
                }

                onCurrentItemChanged: {
                    var sourceIdx = kcm.filteredModel.mapToSource(kcm.filteredModel.index(sourcesList.currentIndex, 0));
                    appConfiguration.rootIndex = kcm.sourcesModel.makePersistentModelIndex(sourceIdx);
                }

                delegate: QtControls.ItemDelegate {
                    id: sourceDelegate

                    width: sourcesList.width
                    text: model.display
                    icon.name: model.decoration
                    highlighted: ListView.isCurrentItem

                    onClicked: {
                        sourcesList.forceActiveFocus();
                        sourcesList.currentIndex = index;
                    }

                    leftPadding: mirrored && indicator.visible ? horizontalPadding + implicitIndicatorWidth + spacing : horizontalPadding
                    rightPadding: !mirrored && indicator.visible ? horizontalPadding + implicitIndicatorWidth + spacing : horizontalPadding

                    indicator: Rectangle {
                        anchors {
                            right: parent.right
                            rightMargin: sourceDelegate.horizontalPadding
                            verticalCenter: parent.verticalCenter
                        }

                        radius: width * 0.5
                        implicitWidth: Kirigami.Units.largeSpacing
                        implicitHeight: Kirigami.Units.largeSpacing
                        visible: kcm.defaultsIndicatorsVisible && !model.isDefault
                        color: Kirigami.Theme.neutralTextColor
                    }
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - (Kirigami.Units.largeSpacing * 4)

                    visible: sourcesList.count === 0 && searchField.length > 0

                    text: i18n("No application or event matches your search term")
                }
            }
        }

        Kirigami.Separator {
            Layout.fillHeight: true
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: Math.round(rootRow.width / 3 * 2)

            ApplicationConfiguration {
                id: appConfiguration
                anchors.fill: parent
                showOnlyEventsConfig: sourcesPage.showOnlyEventsConfig
                visible: appConfiguration.rootIndex.valid
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.largeSpacing * 4)
                text: i18n("Select an application from the list to configure its notification settings and behavior")
                visible: !appConfiguration.rootIndex.valid
            }
        }
    }
}
