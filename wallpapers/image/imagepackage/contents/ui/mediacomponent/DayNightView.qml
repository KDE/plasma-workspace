/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import org.kde.kirigami as Kirigami

StackView {
    id: root

    property var snapshot
    property int fillMode

    property var nextItem: null
    readonly property int status: nextItem ? nextItem.status : Image.Null
    property bool complete: false

    onSnapshotChanged: if (complete) {
        tryReset();
    }
    onFillModeChanged: if (complete) {
        reset();
    }

    Component {
        id: baseImage

        DayNightImage {
            layer.enabled: root.replaceEnter.running
            StackView.onRemoved: destroy()
        }
    }

    replaceEnter: Transition {
        NumberAnimation {
            id: enterAnimation
            property: "opacity"
            from: 0
            to: 1
            duration: Kirigami.Units.veryLongDuration
        }
    }

    replaceExit: Transition {
        PauseAnimation {
            duration: enterAnimation.duration + 500 // 500 to ensure that the previous item doesn't go away before the new item too soon
        }
    }

    function commit(): void {
        if (nextItem.status === Image.Loading) {
            return;
        }

        nextItem.statusChanged.disconnect(root.commit);

        let operation;
        if (empty) {
            operation = StackView.Immediate;
        } else {
            operation = StackView.Transition;
        }

        replace(nextItem, {}, operation);
    }

    function reset(): void {
        if (status === Image.Loading) {
            nextItem.statusChanged.disconnect(root.commit);
        }

        nextItem = baseImage.createObject(root, {
            bottomUrl: snapshot.bottom,
            topUrl: snapshot.top,
            blendFactor: snapshot.blendFactor,
            fillMode: fillMode,
            implicitWidth: root.width,
            implicitHeight: root.height,
            visible: false,
        });
        if (!nextItem) {
            console.warn("Failed to instantiate DayNightImage:", baseImage.errorString());
        }

        if (nextItem.status === Image.Ready) {
            commit();
        } else {
            nextItem.statusChanged.connect(root.commit);
        }
    }

    function tryReset(): void {
        if (snapshot.disjoint) {
            reset();
        } else {
            nextItem.bottomUrl = snapshot.bottom;
            nextItem.topUrl = snapshot.top;
            nextItem.blendFactor = snapshot.blendFactor;
        }
    }

    Component.onCompleted: {
        reset();
        complete = true;
    }
}
