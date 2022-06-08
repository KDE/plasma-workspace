/*
    SPDX-FileCopyrightText: 2022 ivan (@ratijas) tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQml.Models 2.15
import QtQml 2.15

RejectPasswordPathAnimation {
    id: animation

    property list<Item> targets

    // Real target is getting a Translate object which pulls coordinates from
    // a fake Item object. One such object can be reused to multiple targets.
    readonly property Translate __translate: Translate {
        id: translate
        x: fakeTarget.x
    }
    target: Item { id: fakeTarget }

    // We can't bind `transform` list property due to a bunch of bugs and
    // omisions in QML engine, such as the property itself being a
    // non-WRITEable list<> -- so that Binding can't even restore it as a
    // value, and also most Binding properties themselfves including `target`
    // do not have associated NOTIFY signal, so we can't hook into it.
    property list<Item> __oldTargets
    onTargetsChanged: {
        for (let i = 0; i < __oldTargets.length; i++) {
            const target = __oldTargets[i];
            if (target) {
                const list = Array.prototype.slice.call(target.transform);
                const index = list.indexOf(translate);
                if (index !== -1) {
                    list.splice(index, 1);
                    target.transform = list;
                }
            }
        }
        __oldTargets = targets;
        for (let i = 0; i < __oldTargets.length; i++) {
            const target = __oldTargets[i];
            if (target) {
                target.transform.push(translate);
            }
        }
    }
}
