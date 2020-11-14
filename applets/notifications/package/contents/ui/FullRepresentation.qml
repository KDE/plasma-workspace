/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

import QtQuick 2.10
import QtQuick.Layouts 1.1

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as PlasmaComponents // For ModelContextMenu
import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.kirigami 2.12 as Kirigami

import org.kde.kcoreaddons 1.0 as KCoreAddons

import org.kde.notificationmanager 1.0 as NotificationManager

import "global"

PlasmaComponents3.Page {
    // TODO these should be configurable in the future
    readonly property int dndMorningHour: 6
    readonly property int dndEveningHour: 20
    Layout.fillHeight: plasmoid.formFactor === PlasmaCore.Types.Vertical

    // HACK forward focus to the list
    onActiveFocusChanged: {
        if (activeFocus) {
            list.forceActiveFocus();
        }
    }

    Connections {
        target: plasmoid
        function onExpandedChanged() {
            if (plasmoid.expanded) {
                list.positionViewAtBeginning();
                list.currentIndex = -1;
            }
        }
    }

    PlasmaCore.Svg {
        id: lineSvg
        imagePath: "widgets/line"
    }

    header: PlasmaExtras.PlasmoidHeading {
        ColumnLayout {
            anchors {
                fill: parent
                leftMargin: units.smallSpacing
            }
            id: header
            visible: !Kirigami.Settings.isMobile
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: 0

                PlasmaComponents3.CheckBox {
                    id: dndCheck
                    enabled: NotificationManager.Server.valid
                    text: i18n("Do not disturb")
                    icon.name: "notifications-disabled"
                    checkable: true
                    checked: Globals.inhibited

                    // Let the menu open on press
                    onPressed: {
                        if (!Globals.inhibited) {
                            dndMenu.date = new Date();
                            // shows ontop of CheckBox to hide the fact that it's unchecked
                            // until you actually select something :)
                            dndMenu.open(0, 0);
                        }
                    }
                    // but disable only on click
                    onClicked: {
                        if (Globals.inhibited) {
                            Globals.revokeInhibitions();
                        }
                    }


                    PlasmaComponents.ModelContextMenu {
                        id: dndMenu
                        property date date
                        visualParent: dndCheck

                        onClicked: {
                            notificationSettings.notificationsInhibitedUntil = model.date;
                            notificationSettings.save();
                        }

                        model: {
                            var model = [];

                            // For 1 hour
                            var d = dndMenu.date;
                            d.setHours(d.getHours() + 1);
                            d.setSeconds(0);
                            model.push({date: d, text: i18n("For 1 hour")});

                            d = dndMenu.date;
                            d.setHours(d.getHours() + 4);
                            d.setSeconds(0);
                            model.push({date: d, text: i18n("For 4 hours")});

                            // Until this evening
                            if (dndMenu.date.getHours() < dndEveningHour) {
                                d = dndMenu.date;
                                // TODO make the user's preferred time schedule configurable
                                d.setHours(dndEveningHour);
                                d.setMinutes(0);
                                d.setSeconds(0);
                                model.push({date: d, text: i18n("Until this evening")});
                            }

                            // Until next morning
                            if (dndMenu.date.getHours() > dndMorningHour) {
                                d = dndMenu.date;
                                d.setDate(d.getDate() + 1);
                                d.setHours(dndMorningHour);
                                d.setMinutes(0);
                                d.setSeconds(0);
                                model.push({date: d, text: i18n("Until tomorrow morning")});
                            }

                            // Until Monday
                            // show Friday and Saturday, Sunday is "0" but for that you can use "until tomorrow morning"
                            if (dndMenu.date.getDay() >= 5) {
                                d = dndMenu.date;
                                d.setHours(dndMorningHour);
                                // wraps around if necessary
                                d.setDate(d.getDate() + (7 - d.getDay() + 1));
                                d.setMinutes(0);
                                d.setSeconds(0);
                                model.push({date: d, text: i18n("Until Monday")});
                            }

                            // Until "turned off"
                            d = dndMenu.date;
                            // Just set it to one year in the future so we don't need yet another "do not disturb enabled" property
                            d.setFullYear(d.getFullYear() + 1);
                            model.push({date: d, text: i18n("Until manually disabled")});

                            return model;
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                PlasmaComponents3.ToolButton {
                    visible: !(plasmoid.containmentDisplayHints & PlasmaCore.Types.ContainmentDrawsPlasmoidHeading)

                    Accessible.name: plasmoid.action("clearHistory").text
                    icon.name: "edit-clear-history"
                    enabled: plasmoid.action("clearHistory").visible
                    onClicked: action_clearHistory()

                    PlasmaComponents3.ToolTip {
                        text: parent.Accessible.name
                    }
                }
            }

            PlasmaExtras.DescriptiveLabel {
                Layout.leftMargin: dndCheck.mirrored ? 0 : dndCheck.indicator.width + 2 * dndCheck.spacing + units.iconSizes.smallMedium
                Layout.rightMargin: dndCheck.mirrored ? dndCheck.indicator.width + 2 * dndCheck.spacing + units.iconSizes.smallMedium : 0
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
                text: {
                    if (!Globals.inhibited) {
                        return "";
                    }

                    var inhibitedUntil = notificationSettings.notificationsInhibitedUntil;
                    var inhibitedByApp = notificationSettings.notificationsInhibitedByApplication;
                    var inhibitedByMirroredScreens = notificationSettings.inhibitNotificationsWhenScreensMirrored
                                                        && notificationSettings.screensMirrored;

                    var sections = [];

                    // Show until time if valid but not if too far int he future
                    if (!isNaN(inhibitedUntil.getTime()) && inhibitedUntil.getTime() - new Date().getTime() < 100 * 24 * 60 * 60 * 1000 /* 1 year*/) {
                        sections.push(i18nc("Do not disturb until date", "Until %1",
                                            KCoreAddons.Format.formatRelativeDateTime(inhibitedUntil, Locale.ShortFormat)));
                    }

                    if (inhibitedByApp) {
                        var inhibitionAppNames = notificationSettings.notificationInhibitionApplications;
                        var inhibitionAppReasons = notificationSettings.notificationInhibitionReasons;

                        for (var i = 0, length = inhibitionAppNames.length; i < length; ++i) {
                            var name = inhibitionAppNames[i];
                            var reason = inhibitionAppReasons[i];

                            if (reason) {
                                sections.push(i18nc("Do not disturb until app has finished (reason)", "While %1 is active (%2)", name, reason));
                            } else {
                                sections.push(i18nc("Do not disturb until app has finished", "While %1 is active", name));
                            }
                        }
                    }

                    if (inhibitedByMirroredScreens) {
                        sections.push(i18nc("Do not disturb because external mirrored screens connected", "Screens are mirrored"))
                    }

                    return sections.join(" · ");
                }
                visible:  text !== ""
            }
        }
    }

    ColumnLayout{
        // FIXME fix popup size when resizing panel smaller (so it collapses)
        //Layout.preferredWidth: units.gridUnit * 18
        //Layout.preferredHeight: units.gridUnit * 24
        //Layout.minimumWidth: units.gridUnit * 10
        //Layout.minimumHeight: units.gridUnit * 15
        anchors.fill: parent

        spacing: units.smallSpacing

        // actual notifications
        PlasmaExtras.ScrollArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredWidth: units.gridUnit * 18
            Layout.preferredHeight: units.gridUnit * 24
            Layout.leftMargin: units.smallSpacing
            frameVisible: false

            ListView {
                id: list
                model: historyModel
                currentIndex: -1

                Keys.onDeletePressed: {
                    var idx = historyModel.index(currentIndex, 0);
                    if (historyModel.data(idx, NotificationManager.Notifications.ClosableRole)) {
                        historyModel.close(idx);
                        // TODO would be nice to stay inside the current group when deleting an item
                    }
                }
                Keys.onEnterPressed: Keys.onReturnPressed(event)
                Keys.onReturnPressed: {
                    // Trigger default action, if any
                    var idx = historyModel.index(currentIndex, 0);
                    if (historyModel.data(idx, NotificationManager.Notifications.HasDefaultActionRole)) {
                        historyModel.invokeDefaultAction(idx);
                        return;
                    }

                    // Trigger thumbnail URL if there's one
                    var urls = historyModel.data(idx, NotificationManager.Notifications.UrlsRole);
                    if (urls && urls.length === 1) {
                        Qt.openUrlExternally(urls[0]);
                        historyModel.expire(idx);
                        return;
                    }

                    // TODO for finished jobs trigger "Open" or "Open Containing Folder" action
                }
                Keys.onLeftPressed: setGroupExpanded(currentIndex, LayoutMirroring.enabled)
                Keys.onRightPressed: setGroupExpanded(currentIndex, !LayoutMirroring.enabled)

                Keys.onPressed: {
                    switch (event.key) {
                    case Qt.Key_Home:
                        currentIndex = 0;
                        break;
                    case Qt.Key_End:
                        currentIndex = count - 1;
                        break;
                    }
                }

                function isRowExpanded(row) {
                    var idx = historyModel.index(row, 0);
                    return historyModel.data(idx, NotificationManager.Notifications.IsGroupExpandedRole);
                }

                function setGroupExpanded(row, expanded) {
                    var rowIdx = historyModel.index(row, 0);
                    var persistentRowIdx = historyModel.makePersistentModelIndex(rowIdx);
                    var persistentGroupIdx = historyModel.makePersistentModelIndex(historyModel.groupIndex(rowIdx));

                    historyModel.setData(rowIdx, expanded, NotificationManager.Notifications.IsGroupExpandedRole);

                    // If the current item went away when the group collapsed, scroll to the group heading
                    if (!persistentRowIdx || !persistentRowIdx.valid) {
                        if (persistentGroupIdx && persistentGroupIdx.valid) {
                            list.positionViewAtIndex(persistentGroupIdx.row, ListView.Contain);
                            // When closed via keyboard, also set a sane current index
                            if (list.currentIndex > -1) {
                                list.currentIndex = persistentGroupIdx.row;
                            }
                        }
                    }
                }

                highlightMoveDuration: 0
                highlightResizeDuration: 0
                // Not using PlasmaComponents.Highlight as this is only for indicating keyboard focus
                highlight: PlasmaCore.FrameSvgItem {
                    imagePath: "widgets/listitem"
                    prefix: "pressed"
                }

                add: Transition {
                    SequentialAnimation {
                        PropertyAction { property: "opacity"; value: 0 }
                        PauseAnimation { duration: units.longDuration }
                        ParallelAnimation {
                            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: units.longDuration }
                            NumberAnimation { property: "height"; from: 0; duration: units.longDuration }
                        }
                    }
                }
                addDisplaced: Transition {
                    NumberAnimation { properties: "y"; duration:  units.longDuration }
                }

                remove: Transition {
                    id: removeTransition
                    ParallelAnimation {
                        NumberAnimation { property: "opacity"; to: 0; duration: units.longDuration }
                        NumberAnimation {
                            id: removeXAnimation
                            property: "x"
                            to: list.width
                            duration: units.longDuration
                        }
                    }
                }
                removeDisplaced: Transition {
                    SequentialAnimation {
                        PauseAnimation { duration: units.longDuration }
                        NumberAnimation { properties: "y"; duration:  units.longDuration }
                    }
                }

                // This is so the delegates can detect the change in "isInGroup" and show a separator
                section {
                    property: "isInGroup"
                    criteria: ViewSection.FullString
                }

                delegate: DraggableDelegate {
                    id: delegate
                    width: list.width
                    contentItem: delegateLoader

                    draggable: !model.isGroup && model.type != NotificationManager.Notifications.JobType

                    onDismissRequested: {
                        // Setting the animation target explicitly before removing the notification:
                        // Using ViewTransition.item.x to get the x position in the animation
                        // causes random crash in attached property access (cf. Bug 414066)
                        if (x < 0) {
                            removeXAnimation.to = -list.width;
                        }

                        historyModel.close(historyModel.index(index, 0));
                    }

                    Loader {
                        id: delegateLoader
                        width: list.width
                        sourceComponent: model.isGroup ? groupDelegate : notificationDelegate

                        Component {
                            id: groupDelegate
                            NotificationHeader {
                                applicationName: model.applicationName
                                applicationIconSource: model.applicationIconName
                                originName: model.originName || ""

                                // don't show timestamp for group

                                configurable: model.configurable
                                closable: model.closable
                                closeButtonTooltip: i18n("Close Group")

                                onCloseClicked: {
                                    historyModel.close(historyModel.index(index, 0))
                                    if (list.count === 0) {
                                        root.closePassivePlasmoid();
                                    }
                                }

                                onConfigureClicked: historyModel.configure(historyModel.index(index, 0))
                            }
                        }

                        Component {
                            id: notificationDelegate
                            ColumnLayout {
                                spacing: units.smallSpacing

                                RowLayout {
                                    Item {
                                        id: groupLineContainer
                                        Layout.fillHeight: true
                                        Layout.topMargin: units.smallSpacing
                                        width: units.iconSizes.small
                                        visible: model.isInGroup

                                        PlasmaCore.SvgItem {
                                            elementId: "vertical-line"
                                            svg: lineSvg
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            width: units.iconSizes.small
                                            height: parent.height
                                        }
                                    }

                                    NotificationItem {
                                        Layout.fillWidth: true

                                        notificationType: model.type

                                        inGroup: model.isInGroup
                                        inHistory: true

                                        applicationName: model.applicationName
                                        applicationIconSource: model.applicationIconName
                                        originName: model.originName || ""

                                        time: model.updated || model.created

                                        // configure button on every single notifications is bit overwhelming
                                        configurable: !inGroup && model.configurable

                                        dismissable: model.type === NotificationManager.Notifications.JobType
                                            && model.jobState !== NotificationManager.Notifications.JobStateStopped
                                            && model.dismissed
                                            // TODO would be nice to be able to undismiss jobs even when they autohide
                                            && notificationSettings.permanentJobPopups
                                        dismissed: model.dismissed || false
                                        closable: model.closable

                                        summary: model.summary
                                        body: model.body || ""
                                        icon: model.image || model.iconName

                                        urls: model.urls || []

                                        jobState: model.jobState || 0
                                        percentage: model.percentage || 0
                                        jobError: model.jobError || 0
                                        suspendable: !!model.suspendable
                                        killable: !!model.killable
                                        jobDetails: model.jobDetails || null

                                        configureActionLabel: model.configureActionLabel || ""
                                        // In the popup the default action is triggered by clicking on the popup
                                        // however in the list this is undesirable, so instead show a clickable button
                                        // in case you have a non-expired notification in history (do not disturb mode)
                                        // unless it has the same label as an action
                                        readonly property bool addDefaultAction: (model.hasDefaultAction
                                                                                && model.defaultActionLabel
                                                                                && (model.actionLabels || []).indexOf(model.defaultActionLabel) === -1) ? true : false
                                        actionNames: {
                                            var actions = (model.actionNames || []);
                                            if (addDefaultAction) {
                                                actions.unshift("default"); // prepend
                                            }
                                            return actions;
                                        }
                                        actionLabels: {
                                            var labels = (model.actionLabels || []);
                                            if (addDefaultAction) {
                                                labels.unshift(model.defaultActionLabel);
                                            }
                                            return labels;
                                        }

                                        onCloseClicked: {
                                            historyModel.close(historyModel.index(index, 0));
                                            if (list.count === 0) {
                                                root.closePassivePlasmoid();
                                            }
                                        }
                                        onDismissClicked: {
                                            model.dismissed = false;
                                            root.closePassivePlasmoid();
                                        }
                                        onConfigureClicked: historyModel.configure(historyModel.index(index, 0))

                                        onActionInvoked: {
                                            if (actionName === "default") {
                                                historyModel.invokeDefaultAction(historyModel.index(index, 0));
                                            } else {
                                                historyModel.invokeAction(historyModel.index(index, 0), actionName);
                                            }
                                            // Keep it in the history
                                            historyModel.expire(historyModel.index(index, 0));
                                        }
                                        onOpenUrl: {
                                            Qt.openUrlExternally(url);
                                            historyModel.expire(historyModel.index(index, 0));
                                        }
                                        onFileActionInvoked: historyModel.expire(historyModel.index(index, 0))

                                        onSuspendJobClicked: historyModel.suspendJob(historyModel.index(index, 0))
                                        onResumeJobClicked: historyModel.resumeJob(historyModel.index(index, 0))
                                        onKillJobClicked: historyModel.killJob(historyModel.index(index, 0))
                                    }
                                }

                                PlasmaComponents3.ToolButton {
                                    icon.name: model.isGroupExpanded ? "arrow-up" : "arrow-down"
                                    text: model.isGroupExpanded ? i18n("Show Fewer")
                                                                : i18nc("Expand to show n more notifications",
                                                                        "Show %1 More", (model.groupChildrenCount - model.expandedGroupChildrenCount))
                                    visible: (model.groupChildrenCount > model.expandedGroupChildrenCount || model.isGroupExpanded)
                                        && delegate.ListView.nextSection !== delegate.ListView.section
                                    onClicked: list.setGroupExpanded(model.index, !model.isGroupExpanded)
                                }

                                PlasmaCore.SvgItem {
                                    Layout.fillWidth: true
                                    Layout.bottomMargin: units.smallSpacing
                                    elementId: "horizontal-line"
                                    svg: lineSvg

                                    // property is only atached to the delegate itself (the Loader in our case)
                                    visible: (!model.isInGroup || delegate.ListView.nextSection !== delegate.ListView.section)
                                                    && delegate.ListView.nextSection !== "" // don't show after last item
                                }
                            }
                        }
                    }
                }

                PlasmaExtras.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: parent.width - (units.largeSpacing * 4)

                    text: i18n("No unread notifications")
                    visible: list.count === 0 && NotificationManager.Server.valid
                }

                ColumnLayout {
                    id: serverUnavailableColumn

                    width: list.width
                    visible: list.count === 0 && !NotificationManager.Server.valid

                    PlasmaExtras.Heading {
                        Layout.fillWidth: true
                        level: 3
                        opacity: 0.6
                        text: i18n("Notification service not available")
                        wrapMode: Text.WordWrap
                    }

                    PlasmaComponents3.Label {
                        // Checking valid to avoid creating ServerInfo object if everything is alright
                        readonly property NotificationManager.ServerInfo currentOwner: !NotificationManager.Server.valid ? NotificationManager.Server.currentOwner
                                                                                                                        : null

                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: currentOwner ? i18nc("Vendor and product name",
                                            "Notifications are currently provided by '%1 %2'",
                                            currentOwner.vendor,
                                            currentOwner.name)
                                        : ""
                        visible: currentOwner && currentOwner.vendor && currentOwner.name
                    }
                }
            }
        }
    }
}
