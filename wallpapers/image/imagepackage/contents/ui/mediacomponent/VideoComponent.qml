/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.15
import QtMultimedia 5.15

BaseMediaComponent {
    id: videoComponent

    readonly property int status: {
        if (player.error !== MediaPlayer.NoError) {
            return Image.Error;
        }

        switch (player.status) {
        case MediaPlayer.Loaded:
        case MediaPlayer.Buffered:
        case MediaPlayer.EndOfMedia: {
            if (player.playbackState !== MediaPlayer.PlayingState) {
                return Image.Loading;
            }
            return Image.Ready;
        }

        case MediaPlayer.NoMedia:
        case MediaPlayer.InvalidMedia:
        case MediaPlayer.UnknownStatus:
            return Image.Null;
        }
        return Image.Loading;
    }

    blurSource: output

    VideoOutput {
        id: output
        // Manually implement "Centered"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        width: videoComponent.fillMode === Image.Pad ? player.metaData.resolution.width : parent.width
        height: videoComponent.fillMode === Image.Pad ? player.metaData.resolution.height : parent.height

        z: 1

        fillMode: {
            switch (videoComponent.fillMode) {
            case Image.PreserveAspectCrop:
                return VideoOutput.PreserveAspectCrop;
            case Image.Stretch:
                return VideoOutput.Stretch;
            case Image.PreserveAspectFit:
            default:
                return VideoOutput.PreserveAspectFit;
            }
        }
        // Keep lastframe for loop
        flushMode: VideoOutput.LastFrame
        source: player
    }

    /**
     * No pause on window maximized because the player often fails to resume
     * playing.
     *
     * @see https://bugreports.qt.io/browse/QTBUG-104256
     */
    MediaPlayer {
        id: player

        audioRole: MediaPlayer.VideoRole
        autoPlay: true
        loops: 1
        muted: true
        source: videoComponent.source
        volume: 0.0

        /**
         * HACK: Gstreamer backend often stops playing a video at the beginning of a loop.
         * This manually plays the video to create loops.
         *
         * @see https://bugreports.qt.io/browse/QTBUG-104256
         */
        onStopped: {
            player.source = "";
            player.source = videoComponent.source;
            player.play();
        }
    }

    Component.onDestruction: player.source = ""
}
