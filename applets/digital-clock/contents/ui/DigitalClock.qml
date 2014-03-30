/*
 * Copyright 2013 Heena Mahour <heena393@gmail.com>
 * Copyright 2013 Sebastian Kügler <sebas@kde.org>
 * Copyright 2013 Martin Klapetek <mklapetek@kde.org>
 * Copyright 2014 David Edmundson <davidedmundson@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.1
import org.kde.plasma.core 2.0 as PlasmaCore
import org.kde.plasma.components 2.0 as Components

Item {
    id: main

    Layout.minimumWidth: vertical ? 0 : sizehelper.paintedWidth + (units.smallSpacing * 2)
    Layout.maximumWidth: vertical ? Infinity : Layout.minimumWidth
    Layout.preferredWidth: vertical ? undefined : Layout.minimumWidth

    Layout.minimumHeight: vertical ? sizehelper.paintedHeight + (units.smallSpacing * 2) : 0
    Layout.maximumHeight: vertical ? Layout.minimumHeight : Infinity
    Layout.preferredHeight: vertical ? Layout.minimumHeight : theme.mSize(theme.defaultFont).height * 2

    property int formFactor: plasmoid.formFactor
    property int timePixelSize: theme.defaultFont.pixelSize
    property int timezonePixelSize: theme.smallestFont.pixelSize

    property bool constrained: formFactor == PlasmaCore.Types.Vertical || formFactor == PlasmaCore.Types.Horizontal

    property bool vertical: plasmoid.formFactor == PlasmaCore.Types.Vertical

    property bool showSeconds: plasmoid.configuration.showSeconds
    property bool showTimezone: plasmoid.configuration.showTimezone
    property string timeFormat

    onShowSecondsChanged: {
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat))
    }
    onShowTimezoneChanged: {
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat))
    }

    onWidthChanged: geotimer.start()
    onHeightChanged: geotimer.start()

    Connections {
        target: plasmoid.configuration
        onBoldTextChanged: geotimer.start()
        onItalicTextChanged: geotimer.start()
    }

    Timer {
        id: geotimer
        interval: 4 // just to compress resize events of width and height; below 60fps
        onTriggered: {
            if (main.vertical) {
                sizehelper.font.pixelSize = Math.max(theme.mSize(theme.smallestFont).height, Math.min(main.width/5, theme.mSize(theme.defaultFont).height * 2));
            } else if (plasmoid.formFactor == PlasmaCore.Types.Horizontal) {
                sizehelper.font.pixelSize = main.height;
            } else {
                sizehelper.font.pixelSize = Math.min(main.width/5, main.height);
            }
        }
    }

    Components.Label  {
        id: timeLabel
        font {
            weight: plasmoid.configuration.boldText ? Font.Bold : Font.Normal
            italic: plasmoid.configuration.italicText
            pixelSize: sizehelper.font.pixelSize
        }
        width: Math.max(paintedWidth, timeLabel.paintedWidth)
        text: Qt.formatTime(dataSource.data["Local"]["Time"], main.timeFormat);
        wrapMode: plasmoid.formFactor != PlasmaCore.Types.Horizontal ? Text.WordWrap : Text.NoWrap
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: parent.right
            leftMargin: units.smallSpacing
            rightMargin: units.smallSpacing
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                plasmoid.expanded = !plasmoid.expanded;
                calTimer.start();
            }
        }
    }

    Components.Label {
        id: sizehelper
        font.weight: timeLabel.font.weight
        font.italic: timeLabel.font.italic
        wrapMode: plasmoid.formFactor != PlasmaCore.Types.Horizontal ? Text.WordWrap : Text.NoWrap
        visible: false
        anchors {
            verticalCenter: parent.verticalCenter
            left: parent.left
            right: parent.right
            leftMargin: units.smallSpacing
            rightMargin: units.smallSpacing
        }
    }

    // Qt's QLocale does not offer any modular time creating like Klocale did
    // eg. no "gimme time with seconds" or "gimme time without seconds and with timezone".
    // QLocale supports only two formats - Long and Short. Long is unusable in many situations
    // and Short does not provide seconds. So if seconds are enabled, we need to add it here.
    //
    // What happens here is that it looks for the delimiter between "h" and "m", takes it
    // and appends it after "mm" and then appends "ss" for the seconds. Also it checks
    // if the format string already does not contain the seconds part.
    //
    // It can happen that Qt uses the 'C' locale (it's a fallback) and that locale
    // has always ":ss" part in ShortFormat, so we need to remove it.
    function timeFormatCorrection(timeFormatString) {
        if (main.showSeconds && timeFormatString.indexOf('s') == -1) {
            timeFormatString = timeFormatString.replace(/^(hh*)(.+)(mm)(.*?)/i,
                                                        function(match, firstPart, delimiter, secondPart, rest, offset, original) {
                return firstPart + delimiter + secondPart + delimiter + "ss" + rest
            });
        } else if (!main.showSeconds && timeFormatString.indexOf('s') != -1) {
            timeFormatString = timeFormatString.replace(/.ss?/i, "");
        }

        var st = Qt.formatTime(new Date(2000, 0, 1, 10, 0, 0), timeFormatString);
        if (main.showTimezone) {
            st += Qt.formatTime(dataSource.data["Local"]["Time"], " t");
        }


        if (sizehelper.text != st) {
            sizehelper.text = st;
        }

        //FIXME: this always appends the timezone part at the end, it should probably be
        //       Locale-driven, however QLocale does not provide any hint about where to
        //       put it
        if (main.showTimezone && timeFormatString.indexOf('t') == -1) {
            timeFormatString = timeFormatString + " t";
        }


        main.timeFormat = timeFormatString;
    }

    Component.onCompleted: {
        timeFormatCorrection(Qt.locale().timeFormat(Locale.ShortFormat))
    }
}
