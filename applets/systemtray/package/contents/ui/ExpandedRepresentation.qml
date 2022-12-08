/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2020 Nate Graham <nate@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Window 2.15

import org.kde.plasma.core 2.0 as PlasmaCore
// We still need PC2 here for that version of Menu, as PC2 Menu is still very problematic with QActions
// Not being a proper popup window, makes it a showstopper to be used in Plasma
import org.kde.plasma.components 2.0 as PC2
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.extras 2.0 as PlasmaExtras
import org.kde.plasma.plasmoid 2.0

Item {
    id: popup

    Layout.minimumWidth: PlasmaCore.Units.gridUnit * 24
    Layout.maximumWidth: PlasmaCore.Units.gridUnit * 80

    Layout.minimumHeight: PlasmaCore.Units.gridUnit * 24
    Layout.maximumHeight: PlasmaCore.Units.gridUnit * 40

    property alias hiddenLayout: hiddenItemsView.layout
    property alias plasmoidContainer: container

    // Header
    PlasmaExtras.PlasmoidHeading {
        id: plasmoidHeading
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: trayHeading.height + bottomPadding + container.headingHeight
        Behavior on height {
            NumberAnimation { duration: PlasmaCore.Units.shortDuration/2; easing.type: Easing.InOutQuad }
        }
    }

    // Main content layout
    ColumnLayout {
        id: expandedRepresentation
        anchors.fill: parent
        // TODO: remove this so the scrollview fully touches the header;
        // add top padding internally
        spacing: plasmoidHeading.bottomPadding

        // Header content layout
        RowLayout {
            id: trayHeading

            PlasmaComponents.ToolButton {
                id: backButton
                visible: systemTrayState.activeApplet && systemTrayState.activeApplet.expanded && (hiddenLayout.itemCount > 0)
                icon.name: LayoutMirroring.enabled ? "go-previous-symbolic-rtl" : "go-previous-symbolic"

                display: PlasmaComponents.AbstractButton.IconOnly
                text: i18nc("@action:button", "Go Back")

                KeyNavigation.down: hiddenItemsView.visible ? hiddenLayout : container

                onClicked: systemTrayState.setActiveApplet(null)
            }

            PlasmaExtras.Heading {
                Layout.fillWidth: true
                leftPadding: systemTrayState.activeApplet ? 0 : PlasmaCore.Units.smallSpacing * 2

                level: 1
                text: systemTrayState.activeApplet ? systemTrayState.activeApplet.title : i18n("Status and Notifications")
            }

            Repeater {
                id: primaryActionButtons

                model: {
                    const primaryActions = [];
                    actionsButton.applet.contextualActions.forEach(action => {
                        if (action.priority == Plasmoid.HighPriorityAction) {
                            primaryActions.push(action);
                        }
                    })
                    return primaryActions;
                }

                delegate: PlasmaComponents.ToolButton {
                    // We cannot use `action` as it is already a QQuickAction property of the button
                    property QtObject qAction: model.modelData

                    visible: qAction && qAction.visible

                    // NOTE: it needs an IconItem because QtQuickControls2 buttons cannot load QIcons as their icon
                    contentItem: PlasmaCore.IconItem {
                        anchors.centerIn: parent
                        active: parent.hovered
                        implicitWidth: PlasmaCore.Units.iconSizes.smallMedium
                        implicitHeight: implicitWidth
                        source: parent.qAction ? parent.qAction.icon : ""
                    }

                    checkable: qAction && qAction.checkable
                    checked: qAction && qAction.checked
                    display: PlasmaComponents.AbstractButton.IconOnly
                    text: qAction ? qAction.text : ""

                    KeyNavigation.down: backButton.KeyNavigation.down
                    KeyNavigation.left: (index > 0) ? primaryActionButtons.itemAt(index - 1) : backButton
                    KeyNavigation.right: (index < primaryActionButtons.count - 1) ? primaryActionButtons.itemAt(index + 1) :
                                                            actionsButton.visible ? actionsButton : actionsButton.KeyNavigation.right

                    PlasmaComponents.ToolTip {
                        text: parent.text
                    }

                    onClicked: qAction.trigger();
                    onToggled: qAction.toggle();
                }
            }

            PlasmaComponents.ToolButton {
                id: actionsButton
                visible: visibleActions > 0
                checked: visibleActions > 1 ? configMenu.status !== PC2.DialogStatus.Closed : singleAction && singleAction.checked
                property QtObject applet: systemTrayState.activeApplet || plasmoid
                property int visibleActions: menuItemFactory.count
                property QtObject singleAction: visibleActions === 1 && menuItemFactory.object ? menuItemFactory.object.action : null

                icon.name: "application-menu"
                checkable: visibleActions > 1 || (singleAction && singleAction.checkable)
                contentItem.opacity: visibleActions > 1

                display: PlasmaComponents.AbstractButton.IconOnly
                text: actionsButton.singleAction ? actionsButton.singleAction.text : i18n("More actions")

                Accessible.role: actionsButton.singleAction ? Accessible.Button : Accessible.ButtonMenu

                KeyNavigation.down: backButton.KeyNavigation.down
                KeyNavigation.right: configureButton.visible ? configureButton : configureButton.KeyNavigation.right

                // NOTE: it needs an IconItem because QtQuickControls2 buttons cannot load QIcons as their icon
                PlasmaCore.IconItem {
                    parent: actionsButton
                    anchors.centerIn: parent
                    active: actionsButton.hovered
                    implicitWidth: PlasmaCore.Units.iconSizes.smallMedium
                    implicitHeight: implicitWidth
                    source: actionsButton.singleAction !== null ? actionsButton.singleAction.icon : ""
                    visible: actionsButton.singleAction
                }
                onToggled: {
                    if (visibleActions > 1) {
                        if (checked) {
                            configMenu.openRelative();
                        } else {
                            configMenu.close();
                        }
                    }
                }
                onClicked: {
                    if (singleAction) {
                        singleAction.trigger();
                    }
                }
                PlasmaComponents.ToolTip {
                    text: parent.text
                }
                PC2.Menu {
                    id: configMenu
                    visualParent: actionsButton
                    placement: PlasmaCore.Types.BottomPosedLeftAlignedPopup
                }

                Instantiator {
                    id: menuItemFactory
                    model: {
                        configMenu.clearMenuItems();
                        let actions = [];
                        for (let i in actionsButton.applet.contextualActions) {
                            const action = actionsButton.applet.contextualActions[i];
                            if (action.visible
                                    && action.priority > Plasmoid.LowPriorityAction
                                    && !primaryActionButtons.model.includes(action)
                                    && action !== actionsButton.applet.action("configure")) {
                                actions.push(action);
                            }
                        }
                        return actions;
                    }
                    delegate: PC2.MenuItem {
                        id: menuItem
                        action: modelData
                    }
                    onObjectAdded: {
                        configMenu.addMenuItem(object);
                    }
                }
            }
            PlasmaComponents.ToolButton {
                id: configureButton
                icon.name: "configure"
                visible: actionsButton.applet && actionsButton.applet.action("configure")

                display: PlasmaComponents.AbstractButton.IconOnly
                text: actionsButton.applet.action("configure") ? actionsButton.applet.action("configure").text : ""

                KeyNavigation.down: backButton.KeyNavigation.down
                KeyNavigation.left: actionsButton.visible ? actionsButton : actionsButton.KeyNavigation.left
                KeyNavigation.right: pinButton

                PlasmaComponents.ToolTip {
                    text: parent.visible ? parent.text : ""
                }
                onClicked: actionsButton.applet.action("configure").trigger();
            }

            PlasmaComponents.ToolButton {
                id: pinButton
                checkable: true
                checked: Plasmoid.configuration.pin
                onToggled: Plasmoid.configuration.pin = checked
                icon.name: "window-pin"

                display: PlasmaComponents.AbstractButton.IconOnly
                text: i18n("Keep Open")

                KeyNavigation.down: backButton.KeyNavigation.down
                KeyNavigation.left: configureButton.visible ? configureButton : configureButton.KeyNavigation.left

                PlasmaComponents.ToolTip {
                    text: parent.text
                }
            }
        }

        // Grid view of all available items
        HiddenItemsView {
            id: hiddenItemsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: PlasmaCore.Units.smallSpacing
            visible: !systemTrayState.activeApplet

            KeyNavigation.up: pinButton

            onVisibleChanged: {
                if (visible) {
                    layout.forceActiveFocus();
                    systemTrayState.oldVisualIndex = systemTrayState.newVisualIndex = -1;
                }
            }
        }

        // Container for currently visible item
        PlasmoidPopupsContainer {
            id: container
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: systemTrayState.activeApplet

            // We need to add margin on the top so it matches the dialog's own margin
            Layout.topMargin: mergeHeadings ? 0 : dialog.margins.top

            KeyNavigation.up: pinButton
            KeyNavigation.backtab: pinButton

            onVisibleChanged: {
                if (visible) {
                    forceActiveFocus();
                }
            }
        }
    }

    // Footer
    PlasmaExtras.PlasmoidHeading {
        id: plasmoidFooter
        location: PlasmaExtras.PlasmoidHeading.Location.Footer
        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        visible: container.appletHasFooter
        height: container.footerHeight
        // So that it doesn't appear over the content view, which results in
        // the footer controls being inaccessible
        z: -9999
    }
}
