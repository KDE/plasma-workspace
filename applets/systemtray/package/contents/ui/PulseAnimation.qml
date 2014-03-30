/*
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  2.010-1301, USA.
 */

import QtQuick 2.0
//import "Animations.js" as Animations

SequentialAnimation {
    id: pulseAnimation
    objectName: "pulseAnimation"

    property Item targetItem
    //property int duration: Animations.normalDuration
    property int duration: units.longDuration * 6

    onRunningChanged: {
        // Make sure we reset the scale (which is manipulated by the Animation
        // as to avoid freezing a scale icon when the status changes
        if (!running) {
            targetItem.scale = 1.0;
        }
    }
    // Fast scaling while we're animation == more FPS
    ScriptAction { script: { targetItem.smooth = false } }

    SequentialAnimation {

        loops: Animation.Infinite

        PropertyAnimation {
            target: targetItem
            property: "scale"
            from: 1
            to: 1.2
            duration: pulseAnimation.duration * 0.15
            easing.type: Easing.InQuad;
        }

        PropertyAnimation {
            target: targetItem
            property: "scale"
            from: 1.2
            to: 1
            duration: pulseAnimation.duration * 0.15
            easing.type: Easing.OutQuad;
        }

        PauseAnimation {
            duration: pulseAnimation.duration * 0.7
        }
    }

    ScriptAction { script: targetItem.smooth = true }
}


