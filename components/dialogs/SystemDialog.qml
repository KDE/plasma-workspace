/*
 *  SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.plasma.lookandfeel

/**
 * Component to create CSD dialogs that come from the system.
 */
Kirigami.AbstractApplicationWindow {
    id: root

    title: mainText
    /**
     * Main text of the dialog.
     */
    property string mainText: title

    /**
     * Subtitle of the dialog.
     */
    property string subtitle

    /**
     * This property holds the icon used in the dialog.
     */
    property string iconName

    /**
     * This property holds the list of actions for this dialog.
     *
     * Each action will be rendered as a button that the user will be able
     * to click.
     */
    property list<T.Action> actions

    default property Item mainItem

    /**
     * This property holds the QQC2 DialogButtonBox used in the footer of the dialog.
     */
    readonly property T.DialogButtonBox dialogButtonBox: loader.item?.dialogButtonBox ?? null

    /**
     * Provides dialogButtonBox.standardButtons
     *
     * Useful to be able to set it as dialogButtonBox will be null as the object gets built
     */
    property int /*T.DialogButtonBox.StandardButtons*/ standardButtons: T.DialogButtonBox.NoButton

    /**
     * Controls whether the accept button is enabled
     */
    property bool acceptable: true

    /**
     * The layout of the action buttons in the footer of the dialog.
     *
     * By default, if there are more than 3 actions, it will have `Qt.Vertical`.
     *
     * Otherwise, with zero to 2 actions, it will have `Qt.Horizontal`.
     *
     * This will only affect mobile dialogs.
     */
    property int /*Qt.Orientation*/ layout: actions.length > 3 ? Qt.Vertical : Qt.Horizontal

    flags: loader.item?.flags ?? Qt.Dialog
    width: loader.implicitWidth
    height: loader.implicitHeight
    visible: false
    minimumHeight: loader.item?.minimumHeight ?? Kirigami.Units.gridUnit * 10
    minimumWidth: loader.item?.minimumWidth ?? Kirigami.Units.gridUnit * 10

    function present() {
        loader.item.present()
    }

    signal accept()
    signal reject()

    property bool accepted: false

    onAccept: {
        accepted = true
        close()
    }

    onReject: close()

    onVisibleChanged: {
        if (!visible && !accepted) {
            root.reject()
        }
        width = Qt.binding(() => loader.implicitWidth)
        height = Qt.binding(() => loader.implicitHeight)
    }

    Loader {
        id: loader
        anchors.fill: parent
        Component.onCompleted: {
            const component = LookAndFeel.fileUrl("systemdialogscript")
            setSource(component, {
                window: root,
                mainText: root.mainText,
                subtitle: root.subtitle,
                actions: root.actions,
                iconName: root.iconName,
                mainItem: root.mainItem,
                standardButtons: root.standardButtons,
            })
        }

        onItemChanged: root.__bindStandardButtons()

        focus: true

        function accept() {
            const button = root.dialogButtonBox.standardButton(T.DialogButtonBox.Ok);
            if (root.acceptable) {
                root.accept()
            }
        }

        Keys.onEnterPressed: accept()
        Keys.onReturnPressed: accept()
        Keys.onEscapePressed: root.reject()
    }

    Connections {
        // For the lack of better declarative API
        target: loader.item?.dialogButtonBox ?? null
        function onChildrenChanged() {
            root.__bindStandardButtons();
        }
    }

    function __bindStandardButtons() {
        const item = loader.item;
        if (item) {
            const button = item.dialogButtonBox.standardButton(T.DialogButtonBox.Ok);
            if (button) {
                button.enabled = Qt.binding(() => root.acceptable);
            }
        }
    }
}
