/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import org.kde.plasma.workspace.dbus as DBus

TestCase {
    id: root

    when: refreshedSpy.count > 0

    readonly property string service: "org.kde.KSplash"
    readonly property string path: "/KSplash"
    readonly property string iface: service

    DBus.Properties {
        id: props
        busType: DBus.BusType.Session
        service: root.service
        path: root.path
        iface: root.iface

        property int readWriteProp: Number(properties.readWriteProp)
    }

    SignalSpy {
        id: refreshedSpy
        target: props
        signalName: "refreshed"
    }

    SignalSpy {
        id: propertiesChangedSpy
        target: props
        signalName: "propertiesChanged"
    }

    SignalSpy {
        id: readWriteSpy
        target: props
        signalName: "readWritePropChanged"
    }

    function test_readProperty() {
        // Same as the returned value in dbusservice.py
        const args = [
            true,
            new DBus.int16(32767),
            new DBus.int32(2147483647),
            new DBus.int64(21474836470.0),
            new DBus.uint16(65535),
            new DBus.uint32(4294967295),
            new DBus.uint64(18446744073709551615.0),
            new DBus.double(1.23),
            new DBus.byte(255),
            "abc",
            "(bnixqutdysgoa{sv}v)",
            "/abc/def",
            new DBus.dict({
                "int64": new DBus.int16(32767),
                "objectPath": new DBus.objectPath("/abc/def"),
                "string": new DBus.string("string"),
            }),
            new DBus.variant("variant")
        ];
        for (let i = 0; i < args.length; ++i) {
            if (args[i] instanceof DBus.variant) {
                // The decoder returns a value of the actual type so QML can use it directly
                compare(props.properties.readOnlyProp[i], args[i].toString());
            } else if (args[i] instanceof DBus.dict) {
                // The decoder returns QVariantMap so QML can use the dict directly
                console.log(props.properties.readOnlyProp[i], typeof(props.properties.readOnlyProp[i]))
                compare(Number(props.properties.readOnlyProp[i]["int64"]), 32767);
                compare(props.properties.readOnlyProp[i]["objectPath"].toString(), "/abc/def");
                compare(props.properties.readOnlyProp[i]["string"].toString(), "string");
            } else {
                compare(props.properties.readOnlyProp[i], args[i]);
            }
        }
    }

    function test_setProperty() {
        const lastCount = propertiesChangedSpy.count
        compare(props.readWriteProp, 1);
        props.properties.readOnlyProp = 2;
        props.properties.readWriteProp = 3;
        props.properties.readWriteProp = 2;
        compare(props.readWriteProp, 2);
        tryVerify(() => propertiesChangedSpy.count > lastCount);
        tryCompare(props, "readWriteProp", 2);
    }

    function updatePropertySilently() {
        const pendingReply = DBus.SessionBus.asyncCall({
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "updatePropertySilently",
        });
        verify(pendingReply);
        tryVerify(() => pendingReply.isFinished);
    }

    function test_updateProperty() {
        const oldValue = props.readWriteProp;
        updatePropertySilently();
        compare(props.readWriteProp, oldValue);
        readWriteSpy.clear();
        props.update("readWriteProp");
        props.update("readWriteProp"); // Early return
        tryCompare(readWriteSpy, "count", 1);
        compare(props.readWriteProp, oldValue + 1);
    }

    function test_updateAllProperties() {
        const oldValue = props.readWriteProp;
        updatePropertySilently();
        compare(props.readWriteProp, oldValue);
        const lastCount = propertiesChangedSpy.count;
        props.updateAll();
        tryCompare(propertiesChangedSpy, "count", lastCount + 1);
        compare(props.readWriteProp, oldValue + 1);
    }

    function resetService() {
        const lastCount = refreshedSpy.count
        props.service = root.service;
        props.path = root.path;
        props.iface = root.iface;
        tryCompare(refreshedSpy, "count", lastCount + 1);
        verify(props.properties.readOnlyProp != null);
        verify(props.readWriteProp > 0);
    }

    function test_change_service() {
        let lastCount = refreshedSpy.count
        props.iface = "org.foo.bar"
        compare(props.readWriteProp, 0);
        tryCompare(refreshedSpy, "count", lastCount + 1);
        resetService();

        lastCount = refreshedSpy.count
        props.path = "/foo/bar"
        compare(props.readWriteProp, 0);
        tryCompare(refreshedSpy, "count", lastCount + 1);
        resetService();

        lastCount = refreshedSpy.count
        props.service = "org.foo.bar"
        compare(props.readWriteProp, 0);
        tryCompare(refreshedSpy, "count", lastCount + 1);
        resetService();
    }
}
