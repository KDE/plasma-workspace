/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtMultimedia

import org.kde.plasma.wallpapers.image as PlasmaWallpaper
import org.kde.kwindowsystem

BaseMediaComponent {
    id: videoComponent

    readonly property rect desktopRect: Window.window ? Qt.rect(Window.window.x, Window.window.y, Window.window.width, Window.window.height) : Qt.rect(0, 0, 0, 0)
    readonly property int status: {
        if (player.error !== MediaPlayer.NoError) {
            return Image.Error;
        }

        switch (player.mediaStatus) {
        case MediaPlayer.LoadedMedia:
        case MediaPlayer.BufferedMedia:
        case MediaPlayer.EndOfMedia: {
            if (player.playbackState !== MediaPlayer.PlayingState) {
                return Image.Loading;
            }
            return Image.Ready;
        }

        case MediaPlayer.NoMedia:
        case MediaPlayer.InvalidMedia:
            return Image.Null;

        default:
            return Image.Loading;
        }
    }

    blurSource: output

    PlasmaWallpaper.MaximizedWindowMonitor {
        id: activeWindowMonitor
        regionGeometry: videoComponent.desktopRect
        readonly property bool shouldPause: count > 0 && !KWindowSystem.showingDesktop
        onShouldPauseChanged: if (shouldPause) {
            player.pause();
        } else {
            player.play();
        }
    }

    VideoOutput {
        id: output
        // Manually implement "Centered"
        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
        }
        readonly property size resolution: player.metaData.value(MediaMetaData.Resolution) || Qt.size(parent.width, parent.height)
        width: videoComponent.fillMode === Image.Pad ? resolution.width : parent.width
        height: videoComponent.fillMode === Image.Pad ? resolution.height : parent.height

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
    }

    MediaPlayer {
        id: player
        autoPlay: !activeWindowMonitor.shouldPause
        loops: MediaPlayer.Infinite
        source: videoComponent.source
        videoOutput: output
    }
}
