/*
 *  SPDX-FileCopyrightText: 2013 Bhushan Shah <bhush94@gmail.com>
 *  SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 *  SPDX-FileCopyrightText: 2022 ivan tkachenko <me@ratijas.tk>
 *  SPDX-FileCopyrightText: 2023 Mike Noe <noeerover@gmail.com>
 *  SPDX-FileCopyrightText: 2023 Nate Graham <nate@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import Qt5Compat.GraphicalEffects

import org.kde.plasma.components 3.0 as PlasmaComponents3
import org.kde.kirigami 2.20 as Kirigami

/**
 * @brief White text label with a black shadow behind it
 *
 * A standardized label with white text and a black shadow behind it. When using
 * software rendering such that the shadow is not available, a black rounded
 * rectangle is used in its stead.
 *
 * By default it elides text on the right, wraps in a way that prefers word
 * boundaries, and uses plain text formatting.
 *
 * The most important property is "text", which applies to the text property of
 * the underlying Label component. See the Label component from QtQuick.Controls
 * 2 and primitive QML Text element API for additional properties, methods, and
 * signals.
 *
 * @inherit org.kde.plasma.components.Label
 */
PlasmaComponents3.Label {
    /**
     * This property can be used to conditionally *not* render the shadow, even
     * when it's technically possible to render it.
     *
     * default: ``true``
     */
    property bool renderShadow: true

    elide: Text.ElideRight
    wrapMode: Text.WrapAtWordBoundaryOrAnywhere
    textFormat: Text.PlainText

    color: "white"

    layer.enabled: renderShadow && GraphicsInfo.api !== GraphicsInfo.Software
    layer.effect: DropShadow {
        horizontalOffset: 1
        verticalOffset: 1

        radius: 4.0
        samples: radius * 2 + 1
        spread: 0.35
        color: "black"
    }

    // Fallback background when we can't draw the text shadow because hardware
    // rendering isn't available
    Rectangle {
        anchors {
            fill: parent
            margins: -Kirigami.Units.smallSpacing
        }

        color: "black"
        radius: Kirigami.Units.smallSpacing
        opacity: 0.45

        visible: GraphicsInfo.api === GraphicsInfo.Software
    }
}
