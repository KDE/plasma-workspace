/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtGraphicalEffects 1.0

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 3.0 as PlasmaComponents
import org.kde.plasma.private.containmentlayoutmanager 1.0 as ContainmentLayoutManager
import org.kde.kirigami 2.11 as Kirigami

ContainmentLayoutManager.AppletContainer {
    id: appletContainer
    editModeCondition: Plasmoid.immutable
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
            blurEnabled = appletContainer.rotation === 0 && GraphicsInfo.api !== GraphicsInfo.Software && hasElementPrefix("blurred");
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

        // Stored in a property, not as a child, because it is reparented anyway.
        property Item mask: OpacityMask {
            id: mask

            readonly property rect appletContainerScreenRect: {
                const scene = appletContainer.Window.window;
                const position = appletContainer.Kirigami.ScenePosition;
                return clipRect(
                    Qt.rect(
                        position.x,
                        position.y,
                        appletContainer.width,
                        appletContainer.height,
                    ),
                    Qt.size(scene.width, scene.height)
                );
            }

            /** Clip given rectangle to the bounds of given size, assuming bounds position {0,0}.
             * This is a pure library function, similar to QRect::intersected,
             * which Qt should've exposed in QML stdlib.
             */
            function clipRect(rect: rect, bounds: size): rect {
                return Qt.rect(
                    Math.max(0, Math.min(bounds.width, rect.x)),
                    Math.max(0, Math.min(bounds.height, rect.y)),
                    Math.max(0, rect.width
                                + Math.min(0, rect.x)
                                + Math.min(0, bounds.width - (rect.x + rect.width))),
                    Math.max(0, rect.height
                                + Math.min(0, rect.y)
                                + Math.min(0, bounds.height - (rect.y + rect.height))),
                );
            }

            parent: plasmoid
            x: appletContainerScreenRect.x
            y: appletContainerScreenRect.y
            width: appletContainerScreenRect.width
            height: appletContainerScreenRect.height

            rotation: appletContainer.rotation

            visible: background.blurEnabled && (appletContainer.applet.effectiveBackgroundHints & PlasmaCore.Types.StandardBackground)
            enabled: visible
            z: -2
            maskSource: Item {
                // optimized (clipped) blurred-mask

                width: mask.appletContainerScreenRect.width
                height: mask.appletContainerScreenRect.height

                clip: true

                PlasmaCore.FrameSvgItem {
                    imagePath: "widgets/background"
                    prefix: "blurred-mask"

                    x: Math.min(0, appletContainer.Kirigami.ScenePosition.x)
                    y: Math.min(0, appletContainer.Kirigami.ScenePosition.y)

                    width: background.width
                    height: background.height
                }
            }

            source: FastBlur {
                width: mask.appletContainerScreenRect.width
                height: mask.appletContainerScreenRect.height

                radius: 128

                source: ShaderEffectSource {
                    width: mask.appletContainerScreenRect.width
                    height: mask.appletContainerScreenRect.height
                    sourceRect: mask.appletContainerScreenRect
                    sourceItem: appletContainer.Plasmoid.wallpaper
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
