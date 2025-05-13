/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick

Item {
    id: root

    property url bottomUrl
    property url topUrl
    property real blendFactor
    property size sourceSize
    property int fillMode

    readonly property int status: {
        if (firstImage.status === Image.Error || secondImage.status === Image.Error) {
            return Image.Error;
        } else if (firstImage.status === Image.Loading || secondImage.status === Image.Loading) {
            return Image.Loading;
        } else if (firstImage.status === Image.Ready || secondImage.status === Image.Ready) {
            return Image.Ready;
        } else {
            return Image.Null;
        }
    }

    Image {
        id: firstImage
        anchors.fill: parent
        asynchronous: true
        cache: false
        retainWhileLoading: true
        fillMode: root.fillMode
        sourceSize: root.sourceSize
    }

    Image {
        id: secondImage
        anchors.fill: parent
        asynchronous: true
        cache: false
        retainWhileLoading: true
        fillMode: root.fillMode
        sourceSize: root.sourceSize
    }

    function sync(): void {
        let currentItem;
        let nextItem;
        if (firstImage.source === root.bottomUrl) {
            currentItem = firstImage;
            nextItem = secondImage;
        } else if (secondImage.source === root.bottomUrl) {
            currentItem = secondImage;
            nextItem = firstImage;
        } else {
            if (secondImage.source != "") {
                currentItem = secondImage;
                nextItem = firstImage;
            } else {
                currentItem = firstImage;
                nextItem = secondImage;
            }
        }

        currentItem.source = root.bottomUrl;
        nextItem.source = root.topUrl;

        if (root.status === Image.Ready) {
            commit();
        }
    }

    function commit(): void {
        let currentItem;
        if (firstImage.source === root.bottomUrl) {
            currentItem = firstImage;
        } else if (secondImage.source === root.bottomUrl) {
            currentItem = secondImage;
        } else {
            return;
        }

        let nextItem;
        if (firstImage.source === root.topUrl) {
            nextItem = firstImage;
        } else if (secondImage.source === root.topUrl) {
            nextItem = secondImage;
        } else {
            return;
        }

        currentItem.z = 0;
        currentItem.opacity = 1.0;

        nextItem.z = 1;
        nextItem.opacity = root.blendFactor;
    }

    onBottomUrlChanged: Qt.callLater(sync);
    onTopUrlChanged: Qt.callLater(sync);
    onBlendFactorChanged: Qt.callLater(sync);

    onStatusChanged: () => {
        if (status === Image.Ready) {
            commit();
        }
    }

    Component.onCompleted: sync();
}
