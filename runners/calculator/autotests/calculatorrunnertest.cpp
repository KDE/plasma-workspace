/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KRunner/AbstractRunnerTest>
#include <KShell>
#include <QMimeData>
#include <QTest>
#include <libqalculate/includes.h>

using namespace Qt::StringLiterals;

class CalculatorRunnerTest : public KRunner::AbstractRunnerTest
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testQuery();
    void test42();
    void testApproximation();
    void testQuery_data();
#if QALCULATE_MAJOR_VERSION > 2 || QALCULATE_MINOR_VERSION > 6
    void testErrorDetection();
#endif
    void testFunctions();
};

void CalculatorRunnerTest::initTestCase()
{
    initProperties();
}

void CalculatorRunnerTest::testQuery()
{
    QFETCH(QString, query);
    QFETCH(QString, result);

    const auto matches = launchQuery(query);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().text(), result);
}

void CalculatorRunnerTest::testQuery_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QString>("result");

    // clang-format off
    QTest::newRow("simple addition") << u"1+1"_s << u"2"_s;
    QTest::newRow("simple subtraction") << u"2-1"_s << u"1"_s;
    QTest::newRow("simple multiplication") << u"2*2"_s << u"4"_s;
    QTest::newRow("simple division") << u"6/2"_s << u"3"_s;
    QTest::newRow("simple power") << u"2^3"_s << u"8"_s;

    QTest::newRow("x as multiplication sign") << u"25x4"_s << u"100"_s;
    QTest::newRow("single digit factorial") << u"5!"_s << u"120"_s;
    QTest::newRow("superscripted number") << u"2³"_s
                                          << u"8"_s; // BUG: 435932

    QTest::newRow("hex to decimal lower case") << u"0xf"_s << u"15"_s;
    QTest::newRow("hex to decimal upper case") << u"0xF"_s << u"15"_s;
    QTest::newRow("hex to decimal with = at beginning") << u"=0xF"_s << u"15"_s;
    QTest::newRow("decimal to hex") << u"hex=15"_s << u"0xF"_s;
    QTest::newRow("decimal to hex with addition") << u"hex=15+15"_s << u"0x1E"_s;
    QTest::newRow("hex additions") << u"0xF+0xF"_s << u"30"_s;
    QTest::newRow("hex multiplication") << u"0xF*0xF"_s << u"225"_s;
    // BUG: 431362
    QTest::newRow("hex and decimal addition") << u"0x12+1"_s << u"19"_s;
    QTest::newRow("hex and decimal addition reversed") << u"1+0x12"_s << u"19"_s;
    // clang-format on
}

void CalculatorRunnerTest::testApproximation()
{
    const auto matches = launchQuery(u"5^1234567"_s);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().subtext(), u"Approximation"_s);
}

void CalculatorRunnerTest::test42()
{
    auto matches = launchQuery(u"life"_s);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.constFirst().text(), u"42"_s);
    matches = launchQuery(u"universe"_s);
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.constFirst().text(), u"42"_s);
}

#if QALCULATE_MAJOR_VERSION > 2 || QALCULATE_MINOR_VERSION > 6
void CalculatorRunnerTest::testErrorDetection()
{
    launchQuery(u"SDL_VIDEODRIVER="_s);
    QVERIFY(manager->matches().isEmpty());
}
#endif

void CalculatorRunnerTest::testFunctions()
{
    // BUG: 467418
    launchQuery(u"sqrt(4)"_s);
    QCOMPARE(manager->matches().size(), 1);

    launchQuery(u"=sqrt(4)"_s);
    QCOMPARE(manager->matches().size(), 1);

    // Goes to qalculate
    launchQuery(u"=sqrt 4"_s);
    QCOMPARE(manager->matches().size(), 1);

    // Does not match the prefixless queries and function-pattern
    launchQuery(u"sqrt 4"_s);
    QCOMPARE(manager->matches().size(), 0);
}

QTEST_MAIN(CalculatorRunnerTest)

#include "calculatorrunnertest.moc"
