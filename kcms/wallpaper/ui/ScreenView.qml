/*
    SPDX-FileCopyrightText: 2019 Roman Gilg <subdiff@gmail.com>
    SPDX-FileCopyrightText: 2023 Méven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as QQC2
import org.kde.kirigami 2.20 as Kirigami

QQC2.Pane {
    id: pane
    
    signal screenSelected(string screenName)
    
    property var outputs
    property var selectedScreen
    
    readonly property int xOffset: (width - totalSize.width / relativeFactor) / 2;
    readonly property int yOffset: (height - totalSize.height / relativeFactor) / 2;
    
    readonly property rect totalSize: {
        var topleft_x = outputs[0].geometry.x;
        var topleft_y = outputs[0].geometry.y;
        var bottomRight_x = outputs[0].geometry.x + outputs[0].geometry.width;
        var bottomRight_y = outputs[0].geometry.y + outputs[0].geometry.height;
        
        for (let i = 1; i < outputs.length; ++i) {
            var out = outputs[i].geometry
            
            if (out.x < topleft_x) {
                topleft_x = out.x
            }
            if (out.y < topleft_y) {
                topleft_y = out.y
            }
            if (out.x + out.width > bottomRight_x) {
                bottomRight_x = out.x + out.width
            }
            if (out.y + out.height > bottomRight_y) {
                bottomRight_y = out.y + out.height
            }
        }
        return Qt.rect(topleft_x, topleft_y, bottomRight_x - topleft_x, bottomRight_y - topleft_y);
    }
    
    readonly property real relativeFactor: {
        var relativeSize = Qt.size(width === 0 ? 1 : totalSize.width / (0.8 * width),
                                   height === 0 ? 1 : totalSize.height / (0.8 * height));
            if (relativeSize.width > relativeSize.height) {
            // Available width smaller than height, optimize for width (we have
            // '>' because the available width, height is in the denominator).
            return relativeSize.width;
        } else {
            return relativeSize.height;
        }
    }   
    
    Repeater {
        model: outputs
        delegate: Output {
            relativeFactor: pane.relativeFactor
            xOffset: pane.xOffset
            yOffset: pane.yOffset
            screen: outputs[index]
            onScreenSelected: (screenName) => { pane.screenSelected(screenName) }
            isSelected: pane.selectedScreen ? outputs[index].name === pane.selectedScreen.name : false
        }
    }
}
