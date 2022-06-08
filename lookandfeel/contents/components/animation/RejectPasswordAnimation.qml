/*
    SPDX-FileCopyrightText: 2022 ivan (@ratijas) tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick 2.15
import QtQml 2.15

QtObject {
    id: root

    property Item target

    readonly property Animation __animation: RejectPasswordPathAnimation {
        id: animation
        target: Item { id: fakeTarget }
    }

    property Binding __bindEnabled: Binding {
        target: root.target
        property: "enabled"
        value: false
        when: animation.running
        restoreMode: Binding.RestoreBindingOrValue
    }

    // real target is getting a Translate object which pulls coordinates from
    // a fake Item object
    readonly property Translate __translate: Translate {
        id: translate
        x: fakeTarget.x
    }

    // We can't bind `transform` list property due to a bunch of bugs and
    // omisions in QML engine, such as the property itself being a
    // non-WRITEable list<> -- so that Binding can't even restore it as a
    // value, and also most Binding properties themselfves including `target`
    // do not have associated NOTIFY signal, so we can't hook into it.
    property Item __oldTarget
    onTargetChanged: {
        if (__oldTarget !== null) {
            const list = Array.prototype.slice.call(__oldTarget.transform);
            const index = list.indexOf(translate);
            if (index !== -1) {
                list.splice(index, 1);
                __oldTarget.transform = list;
            }
        }
        __oldTarget = target;
        if (__oldTarget !== null) {
            __oldTarget.transform.push(translate);
        }
    }

    function start() {
        animation.start();
    }
}
