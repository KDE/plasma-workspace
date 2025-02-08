/*
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: MIT
*/

pragma ComponentBehavior: Bound

import QtQuick
import QtTest
import org.kde.plasma.workspace.dbus as DBus

TestCase {
    id: root

    readonly property string service: "org.kde.KSplash"
    readonly property string path: "/KSplash"
    readonly property string iface: service

    property DBus.DBusPendingReply pendingReply

    function test_toString_data() {
        return [
            {
                source: [new DBus.byte(107), new DBus.byte(100), new DBus.byte(101)],
                result: "k,d,e"
            },
            {
                source: new DBus.signature("b"),
                result: "b"
            },
            {
                source: new DBus.objectPath("/org/kde/test"),
                result: "/org/kde/test"
            },
            {
                source: new DBus.variant("abc"),
                result: "abc"
            },
        ];
    }

    function test_toString(data) {
        compare(String(data.source), data.result);
    }

    function test_asyncCall_data() {
        // Same as the returned value in dbusservice.py
        const args = [
            new DBus.bool(true),
            new DBus.int16(32767),
            new DBus.int32(2147483647),
            new DBus.int64(21474836470.0),
            new DBus.uint16(65535),
            new DBus.uint32(4294967295),
            new DBus.uint64(18446744073709551615.0),
            new DBus.double(1.23),
            new DBus.byte(255),
            new DBus.string("abc"),
            new DBus.signature("(bnixqutdysgoa{sv}v)"),
            new DBus.objectPath("/abc/def"),
            new DBus.dict({
                "int64": new DBus.int16(32767),
                "objectPath": new DBus.objectPath("/abc/def"),
                "string": new DBus.string("string"),
            }),
            new DBus.variant("variant")
        ];
        const nativeArgs = [
            true,
            1,
            1,
            1,
            1,
            1,
            1,
            1.23,
            255,
            "abc",
            "(bnixqutdysgoa{sv}v)",
            "/abc/def",
            {
                "int64": new DBus.int16(32767),
                "objectPath": new DBus.objectPath("/abc/def"),
                "string": new DBus.string("string"),
            },
            new DBus.variant("variant")
        ];
        const replyArgs = [
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
            {
                "int64": new DBus.int16(32767),
                "objectPath": "/abc/def",
                "string": "string",
            },
            "variant"
        ];
        const listArgs = args.map(v => [v]);
        const nativelistArgs = nativeArgs.map(v => [v]);
        const emptyListArgs = args.map(v => []);
        const replyListArgs = replyArgs.map(v => [v]);
        return [
            {
                member: "testAllTypes",
                arguments: args,
                signature: "(bnixqutdysgoa{sv}v)",
                replyArgument: replyArgs
            },
            {
                member: "testAllTypes",
                arguments: args,
                signature: "", // Deduce the signature automatically
                replyArgument: replyArgs
            },
            {
                member: "testAllTypes",
                arguments: nativeArgs,
                signature: "", // Deduce the signature automatically
                replyArgument: replyArgs
            },
            {
                member: "testListTypes",
                arguments: listArgs,
                signature: "",
                replyArgument: replyListArgs,
            },
            {
                member: "testListTypes",
                arguments: nativelistArgs,
                signature: "",
                replyArgument: replyListArgs,
            },
            {
                member: "testListTypes",
                arguments: emptyListArgs,
                signature: "",
                replyArgument: replyListArgs,
            },
        ];
    }

    function test_asyncCall(data) {
        root.pendingReply = DBus.SessionBus.asyncCall({
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": data.member,
            "arguments": data.arguments,
            "signature": data.signature
        });
        verify(root.pendingReply);
        tryVerify(() => root.pendingReply.isFinished);

        // Received a valid reply
        verify(!root.pendingReply.isError, root.pendingReply.error.message);
        verify(root.pendingReply.isValid);
        compare(root.pendingReply.value, data.replyArgument);
        compare(root.pendingReply.values, [data.replyArgument]);
    }

    function test_ping() {
        root.pendingReply = null;

        const promise = new Promise((resolve, reject) => {
            DBus.SessionBus.asyncCall({
                "service": root.service,
                "path": root.path,
                "iface": root.iface,
                "member": "ping",
            }, resolve, reject);
        }).then((result) => {
            root.pendingReply = result;
        }).catch((error) => {
            root.pendingReply = error;
        });

        tryVerify(() => root.pendingReply);
        verify(() => root.pendingReply.isFinished);

        verify(root.pendingReply.isValid);
        verify(!root.pendingReply.value);
        compare(root.pendingReply.values.length, 0);
    }

    function test_wrong_signature() {
        root.pendingReply = DBus.SessionBus.asyncCall({
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "setStage",
            "signature": "i",
            "arguments": [new DBus.string("break")],
        });
        verify(root.pendingReply);
        tryVerify(() => root.pendingReply.isFinished);

        verify(!root.pendingReply.isValid);
        compare(root.pendingReply.error.name, "org.freedesktop.DBus.Error.InvalidArgs");
    }

    function test_invalid_signature() {
        root.pendingReply = DBus.SessionBus.asyncCall({
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "setStage",
            "signature": "-",
            "arguments": [new DBus.string("break")],
        });
        verify(root.pendingReply);
        tryVerify(() => root.pendingReply.isFinished);

        verify(!root.pendingReply.isValid);
        compare(root.pendingReply.error.name, "org.freedesktop.DBus.Error.InvalidArgs");
    }
}
