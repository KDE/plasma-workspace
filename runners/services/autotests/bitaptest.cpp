// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QObject>
#include <QStandardPaths>
#include <QTest>
#include <QThread>

#include "../bitap.h"

class BitapTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
    }
    void cleanupTestCase()
    {
    }

    void testBitap()
    {
        using namespace Bitap;
        QCOMPARE(bitap(u"hello world", u"hello", 1), 0);
        QCOMPARE(bitap(u"wireshark", u"di", 1), 0);
        QCOMPARE(bitap(u"discover", u"disk", 1), 0);
        QCOMPARE(bitap(u"discover", u"disc", 1), 0);
        QCOMPARE(bitap(u"discover", u"scov", 1), 2);
        QCOMPARE(bitap(u"discover", u"diki", 1), std::nullopt);
        QCOMPARE(bitap(u"discover", u"obo", 1), std::nullopt);
        // With a hamming distance of 1 this should not match. Don't ask me why, I don't know.
        QCOMPARE(bitap(u"discover", u"dicsover", 1), std::nullopt);
        // … but with a distance of 2 it should.
        QCOMPARE(bitap(u"discover", u"dicsover", 2), 0);
        QCOMPARE(bitap(u"discover", u"dicosver", 2), std::nullopt);
        // pattern too long
        QCOMPARE(bitap(u"discover", u" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 1), std::nullopt);
    }
};

QTEST_MAIN(BitapTest)

#include "bitaptest.moc"
