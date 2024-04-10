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
                source: [107 as DBus.byte, 100 as DBus.byte, 101 as DBus.byte],
                result: "k,d,e"
            },
            {
                source: "b" as DBus.signature,
                result: "b"
            },
            {
                source: "/org/kde/test" as DBus.objectPath,
                result: "/org/kde/test"
            },
            {
                source: "abc" as DBus.variant,
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
            true as DBus.bool,
            32767 as DBus.int16,
            2147483647 as DBus.int32,
            21474836470 as DBus.int64,
            65535 as DBus.uint16,
            4294967295 as DBus.uint32,
            18446744073709551615.0 as DBus.uint64,
            1.23 as DBus.double,
            255 as DBus.byte,
            "abc" as DBus.string,
            "(bnixqutdysgoa{sv}v)" as DBus.signature,
            "/abc/def" as DBus.objectPath,
            {
                "int64": 32767 as DBus.int16,
                "objectPath": "/abc/def" as DBus.objectPath,
                "string": "string",
            } as DBus.dict,
            "variant" as DBus.variant
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
                "int64": 32767 as DBus.int16,
                "objectPath": "/abc/def" as DBus.objectPath,
                "string": "string",
            },
            "variant" as DBus.variant
        ];
        const replyArgs = [
            true as DBus.bool,
            32767 as DBus.int16,
            2147483647 as DBus.int32,
            21474836470 as DBus.int64,
            65535 as DBus.uint16,
            4294967295 as DBus.uint32,
            18446744073709551615.0 as DBus.uint64,
            1.23 as DBus.double,
            255 as DBus.byte,
            "abc" as DBus.string,
            "(bnixqutdysgoa{sv}v)" as DBus.signature,
            "/abc/def" as DBus.objectPath,
            {
                "int64": 32767 as DBus.int16,
                "objectPath": "/abc/def" as DBus.objectPath,
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
                signature: "", // Deduct the signature automatically
                replyArgument: replyArgs
            },
            {
                member: "testAllTypes",
                arguments: nativeArgs,
                signature: "", // Deduct the signature automatically
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
        const msg = {
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": data.member,
            "arguments": data.arguments,
            "signature": data.signature
        } as DBus.dbusMessage;

        root.pendingReply = DBus.SessionBus.asyncCall(msg);
        verify(root.pendingReply);
        tryVerify(() => root.pendingReply.isFinished);

        // Received a valid reply
        verify(!root.pendingReply.isError, root.pendingReply.error.message);
        verify(root.pendingReply.isValid);
        compare(root.pendingReply.value, data.replyArgument);
        compare(root.pendingReply.values, [data.replyArgument]);
    }

    function test_ping() {
        const msg = {
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "ping",
        } as DBus.dbusMessage;

        root.pendingReply = DBus.SessionBus.asyncCall(msg);
        verify(root.pendingReply);
        tryVerify(() => root.pendingReply.isFinished);

        verify(root.pendingReply.isValid);
        verify(!root.pendingReply.value);
        compare(root.pendingReply.values.length, 0);
    }

    function test_wrong_signature() {
        const msg = {
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "setStage",
            "signature": "i",
            "arguments": ["break" as DBus.string],
        } as DBus.dbusMessage;
        root.pendingReply = DBus.SessionBus.asyncCall(msg);
        verify(root.pendingReply);
        tryVerify(() => root.pendingReply.isFinished);

        verify(!root.pendingReply.isValid);
        compare(root.pendingReply.error.name, "org.freedesktop.DBus.Error.InvalidArgs");
    }

    function test_invalid_signature() {
        const msg = {
            "service": root.service,
            "path": root.path,
            "iface": root.iface,
            "member": "setStage",
            "signature": "-",
            "arguments": ["break" as DBus.string],
        } as DBus.dbusMessage;
        root.pendingReply = DBus.SessionBus.asyncCall(msg);
        verify(root.pendingReply);
        tryVerify(() => root.pendingReply.isFinished);

        verify(!root.pendingReply.isValid);
        compare(root.pendingReply.error.name, "org.freedesktop.DBus.Error.InvalidArgs");
    }
}
