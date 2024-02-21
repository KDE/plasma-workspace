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
    property string subtitle: ""

    /**
     * This property holds the icon used in the dialog.
     */
    property string iconName: ""

    /**
     * This property holds the list of actions for this dialog.
     *
     * Each action will be rendered as a button that the user will be able
     * to click.
     */
    property list<T.Action> actions

    default property Item mainItem

    /**
     * This property holds the QQC2.DialogButtonBox used in the footer of the dialog.
     */
    readonly property QQC2.DialogButtonBox dialogButtonBox: contentDialog.item.dialogButtonBox

    /**
     * Provides dialogButtonBox.standardButtons
     *
     * Useful to be able to set it as dialogButtonBox will be null as the object gets built
     */
    property var standardButtons: contentDialog.item?.dialogButtonBox.standardButtons

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

    flags: contentDialog.item.flags
    width: contentDialog.implicitWidth
    height: contentDialog.implicitHeight
    visible: false
    minimumHeight: contentDialog.item.minimumHeight
    minimumWidth: contentDialog.item.minimumWidth

    function present() {
        contentDialog.item.present()
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
        width = Qt.binding(() => contentDialog.implicitWidth)
        height = Qt.binding(() => contentDialog.implicitHeight)
    }

    Binding {
        target: dialogButtonBox.standardButton(DialogButtonBox.Ok)
        property: "enabled"
        when: dialogButtonBox.standardButton(DialogButtonBox.Ok)
        value: root.acceptable
    }

    Loader {
        id: contentDialog
        anchors.fill: parent
        Component.onCompleted: {
            var component = LookAndFeel.fileUrl("systemdialogscript")
            var initialProperties = {
                window: root,
                mainText: root.mainText,
                subtitle: root.subtitle,
                actions: root.actions,
                iconName: root.iconName,
                mainItem: root.mainItem,
                standardButtons: root.standardButtons
            };
            setSource(component, initialProperties)
            if (status === Loader.Error) {
                console.warn("Failed loading", component);
                var fallbackComponent = LookAndFeel.fallbackFileUrl("systemdialogscript")
                if (fallbackComponent !== component) {
                    console.warn("Trying fallback file", fallbackComponent)
                    setSource(fallbackComponent, initialProperties)
                }
            }
        }

        focus: true

        function accept() {
            const button = dialogButtonBox.standardButton(DialogButtonBox.Ok);
            if (button && button.enabled) {
                root.accept()
            }
        }
        Keys.onEnterPressed: accept()
        Keys.onReturnPressed: accept()
        Keys.onEscapePressed: root.reject()
    }
}
