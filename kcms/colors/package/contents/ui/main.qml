/*
    SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.6
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import QtQuick.Dialogs 1.0 as QtDialogs
import QtQuick.Controls 2.3 as QtControls
import QtQuick.Templates 2.3 as T
import QtQml 2.15

import org.kde.kirigami 2.8 as Kirigami
import org.kde.newstuff 1.91 as NewStuff
import org.kde.kcm 1.5 as KCM
import org.kde.kquickcontrols 2.0 as KQuickControls
import org.kde.private.kcms.colors 1.0 as Private

KCM.GridViewKCM {
    id: root
    KCM.ConfigModule.quickHelp: i18n("This module lets you choose the color scheme.")

    view.model: kcm.filteredModel
    view.currentIndex: kcm.filteredModel.selectedSchemeIndex

    Binding {
        target: kcm.filteredModel
        property: "query"
        value: searchField.text
        restoreMode: Binding.RestoreBinding
    }

    Binding {
        target: kcm.filteredModel
        property: "filter"
        value:  filterCombo.model[filterCombo.currentIndex].filter
        restoreMode: Binding.RestoreBinding
    }

    KCM.SettingStateBinding {
        configObject: kcm.colorsSettings
        settingName: "colorScheme"
        extraEnabledConditions: !kcm.downloadingFile
    }

    KCM.SettingHighlighter {
        target: accentBox
        highlight: accentBox.checked
    }

    Component.onCompleted: {
        // The thumbnails are a bit more elaborate and need more room, especially when translated
        view.implicitCellWidth = Kirigami.Units.gridUnit * 13;
        view.implicitCellHeight = Kirigami.Units.gridUnit * 12;
    }

    // we have a duplicate property here as "var" instead of "color", so that we
    // can set it to "undefined", which lets us use the "a || b" shorthand for
    // "a if a is defined, otherwise b"
    readonly property var accentColor: Qt.colorEqual(kcm.accentColor, "transparent") ? undefined : kcm.accentColor

    DropArea {
        anchors.fill: parent
        onEntered: {
            if (!drag.hasUrls) {
                drag.accepted = false;
            }
        }
        onDropped: {
            infoLabel.visible = false;
            kcm.installSchemeFromFile(drop.urls[0]);
        }
    }

    // putting the InlineMessage as header item causes it to show up initially despite visible false
    header: ColumnLayout {
        Kirigami.InlineMessage {
            id: notInstalledWarning
            Layout.fillWidth: true

            type: Kirigami.MessageType.Warning
            showCloseButton: true
            visible: false

            Connections {
                target: kcm
                function onShowSchemeNotInstalledWarning(schemeName) {
                    notInstalledWarning.text = i18n("The color scheme '%1' is not installed. Selecting the default theme instead.", schemeName)
                    notInstalledWarning.visible = true;
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
            }

            QtControls.ComboBox {
                id: filterCombo
                textRole: "text"
                model: [
                    {text: i18n("All Schemes"), filter: Private.KCM.AllSchemes},
                    {text: i18n("Light Schemes"), filter: Private.KCM.LightSchemes},
                    {text: i18n("Dark Schemes"), filter: Private.KCM.DarkSchemes}
                ]

                // HACK QQC2 doesn't support icons, so we just tamper with the desktop style ComboBox's background
                // and inject a nice little filter icon.
                Component.onCompleted: {
                    if (!background || !background.hasOwnProperty("properties")) {
                        // not a KQuickStyleItem
                        return;
                    }

                    var props = background.properties || {};

                    background.properties = Qt.binding(function() {
                        var newProps = props;
                        newProps.currentIcon = "view-filter";
                        newProps.iconColor = Kirigami.Theme.textColor;
                        return newProps;
                    });
                }
            }
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true

            QtControls.ButtonGroup {
                buttons: [notAccentBox, wallpaperAccentBox, accentBox]
            }

            QtControls.RadioButton {
                id: notAccentBox

                Kirigami.FormData.label: i18n("Use accent color:")
                text: i18n("From current color scheme")
                checked: Qt.colorEqual(kcm.accentColor, "transparent") && !kcm.accentColorFromWallpaper

                onToggled: {
                    if (checked) {
                        kcm.accentColorFromWallpaper = false;
                        kcm.accentColor = "transparent"
                    }
                }
            }

            QtControls.RadioButton {
                id: wallpaperAccentBox
                text: i18nc("@option:radio wallpaper accent color option", "From current wallpaper")
                checked: kcm.accentColorFromWallpaper

                onToggled: {
                    if (checked) {
                        kcm.accentColorFromWallpaper = true;
                    }
                }
            }

            RowLayout {
                spacing: 0
                QtControls.RadioButton {
                    id: accentBox
                    Layout.alignment: Qt.AlignVCenter
                    checked: !Qt.colorEqual(kcm.accentColor, "transparent") && !kcm.accentColorFromWallpaper

                    onToggled: {
                        if (checked) {
                            kcm.accentColorFromWallpaper = false;
                            kcm.accentColor = Qt.colorEqual(kcm.lastUsedCustomAccentColor, "transparent") ? colorRepeater.model[0] : kcm.lastUsedCustomAccentColor
                        }
                    }
                }

                Flow {
                    id: colourFlow
                    width: root.width - accentBox.width
                    spacing: accentBox.spacing
                    component ColorRadioButton : T.RadioButton {
                        id: control
                        autoExclusive: false

                        property color color: "transparent"

                        implicitWidth: Math.round(Kirigami.Units.gridUnit * 1.25)
                        implicitHeight: Math.round(Kirigami.Units.gridUnit * 1.25)

                        background: Rectangle {
                            readonly property bool showHighlight: parent.hovered && !control.checked && !Qt.colorEqual(control.color, "transparent")
                            color: control.color
                            radius:  showHighlight ? Math.round(height / 4) : Math.round(height / 2)
                            border {
                                color: showHighlight ? Kirigami.Theme.highlightColor : Qt.rgba(0, 0, 0, 0.15)
                            }
                            Behavior on radius {
                                PropertyAnimation {
                                    duration: Kirigami.Units.veryShortDuration
                                    from: Math.round(height / 2)
                                }
                            }
                            Rectangle {
                                id: tabHighlight
                                anchors.fill: parent
                                radius: Math.round(height / 2)
                                scale: 1.3
                                color: "transparent"
                                visible: control.visualFocus
                                border {
                                    color: Kirigami.Theme.highlightColor
                                    width: 1
                                }
                            }
                        }
                        indicator: Rectangle {
                            radius: height / 2
                            visible: control.checked
                            anchors {
                                fill: parent
                                margins: Math.round(Kirigami.Units.smallSpacing * 1.25)
                            }
                            border {
                                color: Qt.rgba(0, 0, 0, 0.15)
                                width: 1
                            }
                        }
                    }

                    Repeater {
                        id: colorRepeater

                        model: [
                            "#e93a9a",
                            "#e93d58",
                            "#e9643a",
                            "#e8cb2d",
                            "#3dd425",
                            "#00d3b8",
                            "#3daee9",
                            "#b875dc",
                            "#926ee4",
                            "#686b6f",
                        ]

                        delegate: ColorRadioButton {
                            color: modelData
                            checked: Qt.colorEqual(kcm.accentColor, modelData)

                            onToggled: {
                                kcm.accentColorFromWallpaper = false;
                                kcm.accentColor = modelData
                                kcm.lastUsedCustomAccentColor = modelData
                                checked = Qt.binding(() => Qt.colorEqual(kcm.accentColor, modelData));
                            }
                        }
                    }

                    QtControls.Label {
                        id: customColorPickerLabel
                        text: i18n("Custom:")
                        opacity: customColorIndicator.opacity
                        Layout.leftMargin: Kirigami.Units.smallSpacing
                    }

                    QtDialogs.ColorDialog {
                        id: colorDialog
                        title: i18n("Choose custom accent color")
                        // User must either choose a colour or cancel the operation before doing something else
                        modality: Qt.ApplicationModal
                        color: Qt.colorEqual(kcm.lastUsedCustomAccentColor, "transparent") ? kcm.accentColor : kcm.lastUsedCustomAccentColor
                        onAccepted: {
                            kcm.accentColor = colorDialog.color
                            kcm.lastUsedCustomAccentColor = colorDialog.color
                            kcm.accentColorFromWallpaper = false
                        }
                    }

                    ColorRadioButton {
                        id: customColorIndicator

                        readonly property bool isCustomColor: !kcm.accentColorFromWallpaper
                            && !Qt.colorEqual(kcm.accentColor, "transparent")
                            && !colorRepeater.model.some(color => Qt.colorEqual(color, root.accentColor))

                       /* The qt binding will keep the binding alive as well as uncheck the button
                        * we can't just disable the button because then the icon will become grey
                        * and also we have to provide a MouseArea for interaction. Both of these
                        * can be done with the button being disabled but it will become very
                        * complex and will result in lot of extra code */

                        function openColorDialog(){
                            checked = Qt.binding(() => customColorIndicator.isCustomColor)
                            colorDialog.open()
                        }

                        color:  isCustomColor && accentBox.checked ? kcm.accentColor : "transparent"
                        checked: isCustomColor

                        onClicked: openColorDialog()

                        QtControls.RoundButton {
                            id: customColorButtonPickerIconContainer

                            anchors.fill: parent
                            padding: 0  // Round button adds some padding by default which we don't need. Setting this to 0 centers the icon

                            visible: !customColorIndicator.isCustomColor

                            onClicked: customColorIndicator.openColorDialog()

                            icon.name: "color-picker"
                            icon.width : Kirigami.Units.iconSizes.small // This provides a nice padding
                        }
                    }
                }
            }
        }
    }

    view.delegate: KCM.GridDelegate {
        id: delegate

        text: model.display

        thumbnailAvailable: true
        thumbnail: Rectangle {
            anchors.fill: parent

            opacity: model.pendingDeletion ? 0.3 : 1
            Behavior on opacity {
                NumberAnimation { duration: Kirigami.Units.longDuration }
            }

            color: kcm.tinted(model.palette.window, kcm.accentColor, model.tints, model.tintFactor)

            Kirigami.Theme.inherit: false
            Kirigami.Theme.highlightColor: root.accentColor || model.palette.highlight
            Kirigami.Theme.textColor: kcm.tinted(model.palette.text, kcm.accentColor, model.tints, model.tintFactor)

            Rectangle {
                id: windowTitleBar
                width: parent.width
                height: Math.round(Kirigami.Units.gridUnit * 1.5)
                color: kcm.tinted((model.accentActiveTitlebar && root.accentColor) ? kcm.accentBackground(root.accentColor, model.palette.window) : model.activeTitleBarBackground, kcm.accentColor, model.tints, model.tintFactor)

                QtControls.Label {
                    anchors {
                        fill: parent
                        leftMargin: Kirigami.Units.smallSpacing
                        rightMargin: Kirigami.Units.smallSpacing
                    }
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: (model.accentActiveTitlebar && root.accentColor) ? kcm.accentForeground(kcm.accentBackground(root.accentColor, model.palette.window), true) : model.activeTitleBarForeground
                    text: i18n("Window Title")
                    elide: Text.ElideRight
                }
            }

            ColumnLayout {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: windowTitleBar.bottom
                    bottom: parent.bottom
                    margins: Kirigami.Units.smallSpacing
                }
                spacing: Kirigami.Units.smallSpacing

                RowLayout {
                    Layout.fillWidth: true

                    QtControls.Label {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        verticalAlignment: Text.AlignVCenter
                        text: i18n("Window text")
                        elide: Text.ElideRight
                        color: model.palette.windowText
                    }

                    QtControls.Button {
                        Layout.alignment: Qt.AlignBottom
                        text: i18n("Button")
                        Kirigami.Theme.inherit: false
                        Kirigami.Theme.highlightColor: kcm.tinted(root.accentColor ? kcm.accentBackground(root.accentColor, model.palette.base) : model.palette.highlight, kcm.accentColor, model.tints, model.tintFactor)
                        Kirigami.Theme.backgroundColor: kcm.tinted(model.palette.button, kcm.accentColor, model.tints, model.tintFactor)
                        Kirigami.Theme.textColor: kcm.tinted(model.palette.buttonText, kcm.accentColor, model.tints, model.tintFactor)
                        activeFocusOnTab: false
                    }
                }

                QtControls.Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    padding: 0

                    activeFocusOnTab: false

                    // Frame by default has a transparent background, override it so we can use the view color
                    // instead.
                    background: Rectangle {
                        color: Kirigami.Theme.backgroundColor
                        border.width: 1
                        border.color: kcm.tinted(Qt.rgba(model.palette.text.r, model.palette.text.g, model.palette.text.b, 0.3), kcm.accentColor, model.tints, model.tintFactor)
                    }

                    // We need to set inherit to false here otherwise the child ItemDelegates will not use the
                    // alternative base color we set here.
                    Kirigami.Theme.inherit: false
                    Kirigami.Theme.backgroundColor: kcm.tinted(model.palette.base, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.disabledTextColor: kcm.tinted(model.disabledText, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.highlightColor: kcm.tinted(root.accentColor ? kcm.accentBackground(root.accentColor, model.palette.base) : model.palette.highlight, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.highlightedTextColor: kcm.tinted(root.accentColor ? kcm.accentForeground(kcm.accentBackground(root.accentColor, model.palette.base), true) : model.palette.highlightedText, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.linkColor: kcm.tinted(root.accentColor || model.palette.link, kcm.accentColor, model.tints, model.tintFactor)
                    Kirigami.Theme.textColor: kcm.tinted(model.palette.text, kcm.accentColor, model.tints, model.tintFactor)
                    Column {
                        id: listPreviewColumn

                        readonly property string demoText: " <a href='#'>%2</a> <a href='#'><font color='%3'>%4</font></a>"
                            .arg(i18nc("Hyperlink", "link"))
                            .arg(model.palette.linkVisited)
                            .arg(i18nc("Visited hyperlink", "visited"))

                        anchors.fill: parent
                        anchors.margins: 1

                        QtControls.ItemDelegate {
                            width: parent.width
                            text: i18n("Normal text") + listPreviewColumn.demoText
                            activeFocusOnTab: false
                        }

                        QtControls.ItemDelegate {
                            width: parent.width
                            highlighted: true
                            // TODO use proper highlighted link color
                            text: i18n("Highlighted text") + listPreviewColumn.demoText
                            activeFocusOnTab: false
                        }

                        QtControls.ItemDelegate {
                            width: parent.width
                            enabled: false
                            text: i18n("Disabled text") + listPreviewColumn.demoText
                            activeFocusOnTab: false
                        }
                    }
                }
            }

            // Make the preview non-clickable but still reacting to hover
            MouseArea {
                anchors.fill: parent
                onClicked: delegate.clicked()
                onDoubleClicked: delegate.doubleClicked()
            }
        }

        actions: [
            Kirigami.Action {
                iconName: "document-edit"
                tooltip: i18n("Edit Color Scheme…")
                enabled: !model.pendingDeletion
                onTriggered: kcm.editScheme(model.schemeName, root)
            },
            Kirigami.Action {
                iconName: "edit-delete"
                tooltip: i18n("Remove Color Scheme")
                enabled: model.removable
                visible: !model.pendingDeletion
                onTriggered: model.pendingDeletion = true
            },
            Kirigami.Action {
                iconName: "edit-undo"
                tooltip: i18n("Restore Color Scheme")
                visible: model.pendingDeletion
                onTriggered: model.pendingDeletion = false
            }
        ]
        onClicked: {
            kcm.model.selectedScheme = model.schemeName;
            view.forceActiveFocus();
        }
        onDoubleClicked: {
            kcm.save();
        }
    }

    footer: ColumnLayout {
        Kirigami.InlineMessage {
            id: infoLabel
            Layout.fillWidth: true

            showCloseButton: true

            Connections {
                target: kcm
                function onShowSuccessMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Positive;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                    // Avoid dual message widgets
                    notInstalledWarning.visible = false;
                }
                function onShowErrorMessage(message) {
                    infoLabel.type = Kirigami.MessageType.Error;
                    infoLabel.text = message;
                    infoLabel.visible = true;
                    notInstalledWarning.visible = false;
                }
            }
        }

        Kirigami.ActionToolBar {
            flat: false
            alignment: Qt.AlignRight
            actions: [
                Kirigami.Action {
                    text: i18n("Install from File…")
                    icon.name: "document-import"
                    onTriggered: fileDialogLoader.active = true
                },
                NewStuff.Action {
                    text: i18n("Get New Color Schemes…")
                    configFile: "colorschemes.knsrc"
                    onEntryEvent: function (entry, event) {
                        if (event == NewStuff.Entry.StatusChangedEvent) {
                            kcm.knsEntryChanged(entry)
                        } else if (event == NewStuff.Entry.AdoptedEvent) {
                            kcm.loadSelectedColorScheme()
                        }
                    }
                }
            ]
        }
    }

    Loader {
        id: fileDialogLoader
        active: false
        sourceComponent: QtDialogs.FileDialog {
            title: i18n("Open Color Scheme")
            folder: shortcuts.home
            nameFilters: [ i18n("Color Scheme Files (*.colors)") ]
            Component.onCompleted: open()
            onAccepted: {
                infoLabel.visible = false;
                kcm.installSchemeFromFile(fileUrls[0])
                fileDialogLoader.active = false
            }
            onRejected: {
                fileDialogLoader.active = false
            }
        }
    }
}
