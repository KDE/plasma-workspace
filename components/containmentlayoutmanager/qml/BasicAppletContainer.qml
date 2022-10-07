/*
    SPDX-FileCopyrightText: 2019 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
    SPDX-FileCopyrightText: 2022 Niccolò Venerandi <niccolo@venerandi.com>

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
    Kirigami.Theme.colorSet: (applet.effectiveBackgroundHints & PlasmaCore.Types.ShadowBackground)
        && !(applet.effectiveBackgroundHints & PlasmaCore.Types.StandardBackground)
        && !(applet.effectiveBackgroundHints & PlasmaCore.Types.TranslucentBackground)
            ? Kirigami.Theme.Complementary
            : Kirigami.Theme.Window

    PlasmaCore.ColorScope.inherit: false
    PlasmaCore.ColorScope.colorGroup: Kirigami.Theme.colorSet === Kirigami.Theme.Complementary
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

        if (applet.preferredRepresentation !== applet.fullRepresentation
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

        if (applet.preferredRepresentation !== applet.fullRepresentation
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
    layer.enabled: (rotation % 90 !== 0) && !(configOverlayItem && configOverlayItem.visible)
    layer.smooth: true

    initialSize.width: applet.switchWidth + leftPadding + rightPadding
    initialSize.height: applet.switchHeight + topPadding + bottomPadding

    background: PlasmaCore.FrameSvgItem {
        id: background

        property bool blurEnabled: false
        property Item maskItem: null

        prefix: blurEnabled ? "blurred" : ""

        imagePath: {
            if (!appletContainer.applet) {
                return "";
            }
            if (appletContainer.applet.effectiveBackgroundHints & PlasmaCore.Types.TranslucentBackground) {
                return "widgets/translucentbackground";
            } else if (appletContainer.applet.effectiveBackgroundHints & PlasmaCore.Types.StandardBackground) {
                return "widgets/background";
            } else {
                return "";
            }
        }

        function bindBlurEnabled() {
            // bind to api and hints automatically, refresh non-observable prefix manually
            blurEnabled = Qt.binding(() =>
                   GraphicsInfo.api !== GraphicsInfo.Software
                && (appletContainer.applet.effectiveBackgroundHints & PlasmaCore.Types.StandardBackground)
                && hasElementPrefix("blurred")
            );
        }

        Component.onCompleted: bindBlurEnabled()
        onRepaintNeeded: bindBlurEnabled()

        onBlurEnabledChanged: {
            if (blurEnabled) {
                if (maskItem === null) {
                    maskItem = maskComponent.createObject(this);
                }
            } else {
                if (maskItem !== null) {
                    maskItem.destroy();
                    maskItem = null;
                }
            }
        }

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

            source: appletContainer.applet && appletContainer.applet.effectiveBackgroundHints & PlasmaCore.Types.ShadowBackground
                ? appletContainer.applet : null
            visible: source !== null
        }
    }

    Component {
        id: maskComponent

        OpacityMask {
            id: mask

            readonly property rect appletContainerScreenRect: {
                const scene = appletContainer.Window.window;
                const position = appletContainer.Kirigami.ScenePosition;
                return clipRect(
                    boundsForTransformedRect(
                        Qt.rect(
                            position.x,
                            position.y,
                            appletContainer.width,
                            appletContainer.height),
                        appletContainer.rotation,
                        appletContainer.scale),
                    Qt.size(scene.width, scene.height));
            }

            /** Apply geometry transformations, and return a bounding rectangle for a resulting shape. */
            // Note: It's basically a custom QMatrix::mapRect implementation, and for
            // simplicity's sake should be replaced when/if mapRect becomes available in QML.
            function boundsForTransformedRect(rect: rect, angle: real, scale: real): rect {
                if (angle === 0 && scale === 1) {
                    return rect; // hot path optimization
                }
                let cosa = Math.abs(Math.cos(angle * (Math.PI / 180))) * scale;
                let sina = Math.abs(Math.sin(angle * (Math.PI / 180))) * scale;
                let newSize = Qt.size(
                    rect.width * cosa + rect.height * sina,
                    rect.width * sina + rect.height * cosa);
                return Qt.rect(
                    rect.left + (rect.width - newSize.width) / 2,
                    rect.top + (rect.height - newSize.height) / 2,
                    newSize.width,
                    newSize.height);
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

            parent: appletContainer.Plasmoid.self
            x: appletContainerScreenRect.x
            y: appletContainerScreenRect.y
            width: appletContainerScreenRect.width
            height: appletContainerScreenRect.height

            z: -2
            maskSource: Item {
                // optimized (clipped) blurred-mask

                width: mask.appletContainerScreenRect.width
                height: mask.appletContainerScreenRect.height

                clip: true

                PlasmaCore.FrameSvgItem {
                    imagePath: "widgets/background"
                    prefix: "blurred-mask"

                    x: appletContainer.Kirigami.ScenePosition.x - mask.appletContainerScreenRect.x
                    y: appletContainer.Kirigami.ScenePosition.y - mask.appletContainerScreenRect.y

                    width: background.width
                    height: background.height

                    rotation: appletContainer.rotation
                    scale: appletContainer.scale
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
