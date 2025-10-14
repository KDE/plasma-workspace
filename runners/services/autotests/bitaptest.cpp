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
        // The macro has trouble with designated initializers, so we wrap them in ().
        QCOMPARE(bitap(u"hello world", u"hello", 1), (Match{.size = 5, .distance = 0}));
        QCOMPARE(bitap(u"wireshark", u"di", 1), (Match{.size = 2, .distance = 1}));
        QCOMPARE(bitap(u"discover", u"disk", 1), (Match{.size = 3, .distance = 1}));
        QCOMPARE(bitap(u"discover", u"disc", 1), (Match{.size = 4, .distance = 0}));
        QCOMPARE(bitap(u"discover", u"scov", 1), (Match{.size = 4, .distance = 0}));
        QCOMPARE(bitap(u"discover", u"diki", 1), std::nullopt);
        QCOMPARE(bitap(u"discover", u"obo", 1), std::nullopt);
        // With a hamming distance of 1 this may match because it is a single transposition.
        QCOMPARE(bitap(u"discover", u"dicsover", 1), (Match{.size = 8, .distance = 1}));
        // … but with three characters out of place things should not match.
        QCOMPARE(bitap(u"discover", u"dicosver", 1), std::nullopt);
        // pattern too long
        QCOMPARE(bitap(u"discover", u" !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~", 1), std::nullopt);
        // This is not a transposition as per Damerau–Levenshtein distance because the characters are not adjacent.
        QCOMPARE(bitap(u"steam", u"skeap", 1), std::nullopt);
        // Deletion required
        QCOMPARE(bitap(u"discover", u"discover", 1), (Match{.size = 8, .distance = 0}));
        QCOMPARE(bitap(u"discover", u"discovery", 1), (Match{.size = 8, .distance = 1}));
        // Insertion required

        QCOMPARE(bitap(u"discover", u"dicover", 1), (Match{.size = 7, .distance = 1}));
        // Would have to change the entire pattern -> no match
        QCOMPARE(bitap(u"discover", u";", 1), std::nullopt);

        // RKWard shouldn't be more relevant because the "d" matches towards the end
        QCOMPARE(bitap(u"dolphin", u"do", 1), (Match{.size = 2, .distance = 0}));
        QCOMPARE(bitap(u"RKWard", u"do", 1), (Match{.size = 2, .distance = 1}));
    }

    void testScore()
    {
        using namespace Bitap;
        // aperfectten has 10 big beautiful indexes. The maximum end is therefore 10.
        QCOMPARE(score(u"aperfectten", Match{.size = 11, .distance = 0}, 1), 1.0);
        QCOMPARE(score(u"aperfectten", Match{.size = 5, .distance = 0}, 1), 5.0 / 11.0); // 0.454545...
        QCOMPARE(score(u"aperfectten", Match{.size = 5, .distance = 1}, 1), (5.0 / 11.0) - 0.05); // 0.404545...
        QCOMPARE(score(u"aperfectten", Match{.size = 0, .distance = 0}, 0), 0);
        QCOMPARE(score(u"aperfectten", Match{.size = 0, .distance = 0}, 1), 0);
        QCOMPARE(score(u"aperfectten", Match{.size = 2, .distance = 1}, 1), (2.0 / 11) - 0.05);

        QCOMPARE(score(u"abc", Match{.size = 3, .distance = 1}, 1), 0.95);
        // Ask for distance 0 but it has a distance so this is a super bad match.
        QCOMPARE(score(u"abc", Match{.size = 3, .distance = 1}, 0), 0);
    }
};

QTEST_MAIN(BitapTest)

#include "bitaptest.moc"
