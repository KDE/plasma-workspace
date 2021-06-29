/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.12
import QtQuick.Layouts 1.2
import QtGraphicalEffects 1.0

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.private.containmentlayoutmanager 1.0 as ContainmentLayoutManager
import org.kde.kirigami 2.11 as Kirigami

ContainmentLayoutManager.AppletContainer {
    id: appletContainer
    editModeCondition: plasmoid.immutable
        ? ContainmentLayoutManager.ItemContainer.Manual
        : ContainmentLayoutManager.ItemContainer.AfterPressAndHold

    Kirigami.Theme.inherit: false
    Kirigami.Theme.colorSet: (contentItem.effectiveBackgroundHints & PlasmaCore.Types.ShadowBackground)
        && !(contentItem.effectiveBackgroundHints & PlasmaCore.Types.StandardBackground)
        && !(contentItem.effectiveBackgroundHints & PlasmaCore.Types.TranslucentBackground)
            ? Kirigami.Theme.Complementary
            : Kirigami.Theme.Window

    PlasmaCore.ColorScope.inherit: false
    PlasmaCore.ColorScope.colorGroup: Kirigami.Theme.colorSet == Kirigami.Theme.Complementary
        ? PlasmaCore.Theme.ComplementaryColorGroup
        : PlasmaCore.Theme.NormalColorGroup

    onFocusChanged: {
        if (!focus && !dragActive) {
            editMode = false;
        }
    }
    Layout.minimumWidth: {
        if (!applet) {
            return leftPadding + rightPadding;
        }

        if (applet.preferredRepresentation != applet.fullRepresentation
            && applet.compactRepresentationItem
        ) {
            return applet.compactRepresentationItem.Layout.minimumWidth + leftPadding + rightPadding;
        } else {
            return applet.Layout.minimumWidth + leftPadding + rightPadding;
        }
    }
    Layout.minimumHeight: {
        if (!applet) {
            return topPadding + bottomPadding;
        }

        if (applet.preferredRepresentation != applet.fullRepresentation
            && applet.compactRepresentationItem
        ) {
            return applet.compactRepresentationItem.Layout.minimumHeight + topPadding + bottomPadding;
        } else {
            return applet.Layout.minimumHeight + topPadding + bottomPadding;
        }
    }

    Layout.preferredWidth: Math.max(applet.Layout.minimumWidth, applet.Layout.preferredWidth)
    Layout.preferredHeight: Math.max(applet.Layout.minimumHeight, applet.Layout.preferredHeight)

    Layout.maximumWidth: applet.Layout.maximumWidth
    Layout.maximumHeight: applet.Layout.maximumHeight

    leftPadding: background.margins.left
    topPadding: background.margins.top
    rightPadding: background.margins.right
    bottomPadding: background.margins.bottom

    // render via a layer if we're at an angle
    // resize handles are rendered outside this item, so also disable when they're showing to avoid clipping
    layer.enabled: (rotation % 90 != 0) && !(configOverlayItem && configOverlayItem.visible)
    layer.smooth: true

    initialSize.width: applet.switchWidth + leftPadding + rightPadding
    initialSize.height: applet.switchHeight + topPadding + bottomPadding

    onRotationChanged: background.syncBlurEnabled()

    background: PlasmaCore.FrameSvgItem {
        id: background
        imagePath: {
            if (!contentItem) {
                return "";
            }
            if (contentItem.effectiveBackgroundHints & PlasmaCore.Types.TranslucentBackground) {
                return "widgets/translucentbackground";
            } else if (contentItem.effectiveBackgroundHints & PlasmaCore.Types.StandardBackground) {
                return "widgets/background";
            } else {
                return "";
            }
        }

        property bool blurEnabled: false
        function syncBlurEnabled() {
            blurEnabled = appletContainer.rotation === 0 && plasmoid.GraphicsInfo.api !== GraphicsInfo.Software && hasElementPrefix("blurred");
        }
        prefix: blurEnabled ? "blurred" : ""
        Component.onCompleted: syncBlurEnabled()

        onRepaintNeeded: syncBlurEnabled()

        DropShadow {
            anchors {
                fill: parent
                leftMargin: appletContainer.leftPadding
                topMargin: appletContainer.topPadding
                rightMargin: appletContainer.rightPadding
                bottomMargin: appletContainer.bottomPadding
            }
            z: -1
            horizontalOffset: 0
            verticalOffset: 1

            radius: 4
            samples: 9
            spread: 0.35

            color: Qt.rgba(0, 0, 0, 0.5)
            opacity: 1

            source: contentItem && contentItem.effectiveBackgroundHints & PlasmaCore.Types.ShadowBackground ? contentItem : null
            visible: source != null
        }

        OpacityMask {
            id: mask
            enabled: visible
            rotation: appletContainer.rotation
            Component.onCompleted:  mask.parent = plasmoid
            width: appletContainer.width
            height: appletContainer.height
            x: appletContainer.Kirigami.ScenePosition.x + Math.max(0, -appletContainer.x)
            y: appletContainer.Kirigami.ScenePosition.y + Math.max(0, -appletContainer.y)

            visible: background.blurEnabled && (appletContainer.applet.effectiveBackgroundHints & PlasmaCore.Types.StandardBackground)
            z: -2
            source: blur
            maskSource: 
            ShaderEffectSource {
                width: mask.width
                height: mask.height
                sourceRect: Qt.rect(Math.max(0, -appletContainer.x),
                                    Math.max(0, -appletContainer.y),
                                    width, height);
                sourceItem: PlasmaCore.FrameSvgItem {
                    imagePath: "widgets/background"
                    prefix: "blurred-mask"
                    parent: appletContainer.background
                    anchors.fill: parent
                    visible: false
                }
            }

            FastBlur {
                id: blur
                anchors.fill: parent

                radius: 128
                visible: false

                source: ShaderEffectSource {
                    width: blur.width
                    height: blur.height
                    sourceRect: Qt.rect(Math.max(0, appletContainer.x),
                                        Math.max(0, appletContainer.y),
                                        appletContainer.width - Math.max(0, - (appletContainer.parent.width - appletContainer.x - appletContainer.width)),
                                        appletContainer.height - Math.max(0, - (appletContainer.parent.height - appletContainer.y - appletContainer.height)));
                    sourceItem: plasmoid.wallpaper
                }
            }
        }
    }

    busyIndicatorComponent: PlasmaComponents.BusyIndicator {
        anchors.centerIn: parent
        visible: applet.busy
        running: visible
    }
    configurationRequiredComponent: PlasmaComponents.Button {
        anchors.centerIn: parent
        text: i18n("Configure…")
        icon.name: "configure"
        visible: applet.configurationRequired
        onClicked: applet.action("configure").trigger();
    }
} 
