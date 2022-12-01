/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.9
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QtControls
import QtQml 2.15

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kcm 1.2 as KCM

import org.kde.private.kcms.notifications 1.0 as Private

Kirigami.Page {
    id: sourcesPage
    title: i18n("Application Settings")

    padding: Kirigami.Units.mediumSpacing

    Component.onCompleted: {
        var idx = kcm.sourcesModel.persistentIndexForDesktopEntry(kcm.initialDesktopEntry);
        if (!idx.valid) {
            idx = kcm.sourcesModel.persistentIndexForNotifyRcName(kcm.initialNotifyRcName);
        }
        appConfiguration.rootIndex = idx;

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

    RowLayout {
        id: rootRow
        anchors.fill: parent

        ColumnLayout {
            Layout.minimumWidth: Kirigami.Units.gridUnit * 12
            Layout.preferredWidth: Math.round(rootRow.width / 3)

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
            }

            QtControls.ScrollView {
                id: sourcesScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                Kirigami.Theme.inherit: false

                Component.onCompleted: background.visible = true

                ListView {
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

                    delegate: Kirigami.BasicListItem {
                        id: sourceDelegate
                        width: sourcesList.width
                        text: model.display
                        icon: model.decoration
                        highlighted: ListView.isCurrentItem
                        onClicked: {
                            sourcesList.forceActiveFocus();
                            sourcesList.currentIndex = index;
                        }
                        Rectangle {
                            id: defaultIndicator
                            radius: width * 0.5
                            implicitWidth: Kirigami.Units.largeSpacing
                            implicitHeight: Kirigami.Units.largeSpacing
                            visible: kcm.defaultsIndicatorsVisible
                            opacity: !model.isDefault
                            color: Kirigami.Theme.neutralTextColor
                        }
                    }

                    Kirigami.PlaceholderMessage {
                        anchors.centerIn: parent
                        width: parent.width - (Kirigami.Units.largeSpacing * 4)

                        visible: sourcesList.count === 0 && searchField.length > 0

                        text: i18n("No application or event matches your search term.")
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: Math.round(rootRow.width / 3 * 2)

            ApplicationConfiguration {
                id: appConfiguration
                anchors.fill: parent
                visible: typeof appConfiguration.rootIndex !== "undefined" && appConfiguration.rootIndex.valid
            }

            Kirigami.PlaceholderMessage {
                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.largeSpacing * 4)
                text: i18n("Select an application from the list to configure its notification settings and behavior.")
                visible: !appConfiguration.rootIndex || !appConfiguration.rootIndex.valid
            }
        }
    }
}
