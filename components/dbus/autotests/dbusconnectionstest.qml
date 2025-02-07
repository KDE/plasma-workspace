/*
    SPDX-FileCopyrightText: 2025 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

pragma ComponentBehavior: Bound
pragma ValueTypeBehavior: Addressable

import QtQuick
import QtTest
import org.kde.plasma.workspace.dbus as DBus

TestCase {
    id: root

    readonly property string service: "org.kde.KSplash"
    readonly property string path: "/KSplash"
    readonly property string iface: service

    signal pong()

    DBus.SignalWatcher {
        id: connections
        busType: DBus.BusType.Session
        service: root.service
        path: root.path
        iface: root.iface

        property bool b
        property int n
        property int i
        property double x
        property int q
        property double u
        property DBus.uint64 t
        property double d
        property var y
        property string s
        property string g
        property string o
        property var asv
        property var v

        function dbuspong(b, n, i, x, q, u, t, d, y, s, g, o, asv, v) {
            connections.b = b;
            connections.n = Number(n);
            connections.i = Number(i);
            connections.x = Number(x);
            connections.q = Number(q);
            connections.u = Number(u);
            connections.t = t;
            connections.d = Number(d);
            connections.y = y;
            connections.s = String(s);
            connections.g = String(g);
            connections.o = String(o);
            connections.asv = asv;
            connections.v = v;
            root.pong();
        }
    }

    SignalSpy {
        id: pongSpy
        target: root
        signalName: "pong"
    }

    function ping() {
        return DBus.SessionBus.asyncCall({
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "ping",
        });
    }

    function test_emitSignal() {
        const lastCount = pongSpy.count;
        ping();
        tryVerify(() => pongSpy.count > lastCount);
        compare(connections.b, true);
        compare(connections.n, 32767);
        compare(connections.i, 2147483647);
        compare(connections.x, 21474836470.0);
        compare(connections.q, 65535);
        compare(connections.u, 4294967295.0);
        compare(connections.t, new DBus.uint64(18446744073709551615.0));
        compare(connections.d, 1.23);
        compare(connections.y, new DBus.byte(255));
        compare(connections.s, "abc");
        compare(connections.g, "(bnixqutdysgoa{sv}v)");
        compare(connections.o, "/abc/def");
        compare(Number(connections.asv["int64"]), 32767);
        compare(String(connections.asv["objectPath"]), "/abc/def");
        compare(connections.asv["string"], "string");
        compare(connections.v, "variant");
    }

    function resetService() {
        connections.enabled = true;
        connections.busType = DBus.BusType.Session;
        connections.service = root.service;
        connections.path = root.path;
        connections.iface = root.iface;
    }

    function test_change_service() {
        let lastCount = pongSpy.count;

        connections.busType = "foobar";
        compare(connections.busType, "foobar");
        resetService();
        ping();
        tryCompare(pongSpy, "count", lastCount + 1);

        lastCount = lastCount + 1;
        connections.iface = "org.foo.bar";
        resetService();
        ping();
        tryCompare(pongSpy, "count", lastCount + 1);

        lastCount = lastCount + 1;
        connections.path = "/foo/bar";
        resetService();
        ping();
        tryCompare(pongSpy, "count", lastCount + 1);

        lastCount = lastCount + 1;
        connections.service = "org.foo.bar";
        resetService();
        ping();
        tryCompare(pongSpy, "count", lastCount + 1);

        lastCount = lastCount + 1;
        connections.enabled = false;
        resetService();
        ping();
        tryCompare(pongSpy, "count", lastCount + 1);
    }
}
