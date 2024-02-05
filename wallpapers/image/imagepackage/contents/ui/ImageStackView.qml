/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.plasma.wallpapers.image as Wallpaper
import org.kde.kirigami as Kirigami

QQC2.StackView {
    id: view

    required property int fillMode
    required property string configColor
    required property bool blur
    property alias source: mediaProxy.source
    required property size sourceSize
    required property QtObject wallpaperInterface

    readonly property alias mediaProxy: mediaProxy
    readonly property url modelImage: mediaProxy.modelImage

    /**
     * Stores pending image here to avoid the default image overriding the true image.
     *
     * @see BUG 456189
     */
    property Item pendingImage

    property bool doesSkipAnimation: true

    property Component staticImageComponent
    property Component animatedImageComponent

    onFillModeChanged: Qt.callLater(loadImage);
    onModelImageChanged: Qt.callLater(loadImage);
    onConfigColorChanged: Qt.callLater(loadImage);
    onBlurChanged: Qt.callLater(loadImage);

    function createBackgroundComponent() {
        switch (mediaProxy.backgroundType) {
        case Wallpaper.BackgroundType.Image: {
            if (!staticImageComponent) {
                staticImageComponent = Qt.createComponent("mediacomponent/StaticImageComponent.qml");
            }
            return staticImageComponent;
        }
        case Wallpaper.BackgroundType.AnimatedImage: {
            if (!animatedImageComponent) {
                animatedImageComponent = Qt.createComponent("mediacomponent/AnimatedImageComponent.qml");
            }
            return animatedImageComponent;
        }
        }
    }

    function loadImageImmediately() {
        loadImage(true);
    }

    function loadImage(skipAnimation) {
        if (pendingImage) {
            pendingImage.statusChanged.disconnect(replaceWhenLoaded);
            pendingImage.destroy();
            pendingImage = null;
        }

        if (mediaProxy.providerType == Wallpaper.Provider.Unknown) {
            console.error("The backend got an unknown wallpaper provider type. The wallpaper will now fall back to the default. Please check your wallpaper configuration!");
            mediaProxy.useSingleImageDefaults();
            return;
        }

        doesSkipAnimation = view.currentItem == undefined || view.currentItem.sourceSize !== view.sourceSize || !!skipAnimation;
        const baseImage = createBackgroundComponent();
        pendingImage = baseImage.createObject(view, {
            // Use mediaProxy instead of view because colorSchemeChanged needs immediately update the wallpaper
            "source": mediaProxy.modelImage,
            "fillMode": view.fillMode,
            "sourceSize": view.sourceSize,
            "color": view.configColor,
            "blur": view.blur,
            "opacity": 0,
            "width": view.width,
            "height": view.height,
        });

        pendingImage.statusChanged.connect(replaceWhenLoaded);
        replaceWhenLoaded();
    }

    function replaceWhenLoaded() {
        if (pendingImage.status === Image.Loading) {
            return;
        }

        pendingImage.statusChanged.disconnect(replaceWhenLoaded);
        // BUG 454908: Update accent color
        pendingImage.QQC2.StackView.onActivated.connect(() => {
            if (Qt.colorEqual(mediaProxy.customColor, "transparent") && Qt.colorEqual(wallpaperInterface.accentColor, "transparent")) {
                wallpaperInterface.accentColorChanged();
            } else {
                wallpaperInterface.accentColor = mediaProxy.customColor;
            }
        });

        // onRemoved only fires when all transitions end. If a user switches wallpaper quickly this adds up
        // Given it's such a heavy item, try to cleanup as early as possible
        pendingImage.QQC2.StackView.onDeactivated.connect(pendingImage.destroy);
        pendingImage.QQC2.StackView.onRemoved.connect(pendingImage.destroy);
        view.replace(pendingImage, {}, QQC2.StackView.Transition);

        wallpaperInterface.loading = false;

        if (pendingImage.status !== Image.Ready) {
            mediaProxy.useSingleImageDefaults();
        }

        pendingImage = null;
    }

    replaceEnter: Transition {
        OpacityAnimator {
            id: replaceEnterOpacityAnimator
            from: 0
            to: 1
            // The value is to keep compatible with the old feeling defined by "TransitionAnimationDuration" (default: 1000)
            // 1 is HACK for https://bugreports.qt.io/browse/QTBUG-106797 to avoid flickering
            duration: view.doesSkipAnimation ? 1 : Math.round(Kirigami.Units.veryLongDuration * 2.5)
        }
    }
    // Keep the old image around till the new one is fully faded in
    // If we fade both at the same time you can see the background behind glimpse through
    replaceExit: Transition{
        PauseAnimation {
            // 500: The exit transition starts first and can be completed earlier than the enter transition
            duration: replaceEnterOpacityAnimator.duration + 500
        }
    }

    Wallpaper.MediaProxy {
        id: mediaProxy

        targetSize: view.sourceSize

        onActualSizeChanged: Qt.callLater(loadImageImmediately);
        onColorSchemeChanged: loadImageImmediately();
        onSourceFileUpdated: loadImageImmediately()
    }
}
