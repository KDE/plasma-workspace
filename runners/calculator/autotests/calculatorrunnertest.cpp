/*
    SPDX-FileCopyrightText: 2020 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <KRunner/AbstractRunnerTest>
#include <KShell>
#include <QMimeData>
#include <QTest>
#include <libqalculate/includes.h>

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
    QTest::newRow("simple addition") << "1+1" << "2";
    QTest::newRow("simple subtraction") << "2-1" << "1";
    QTest::newRow("simple multiplication") << "2*2" << "4";
    QTest::newRow("simple division") << "6/2" << "3";
    QTest::newRow("simple power") << "2^3" << "8";

    QTest::newRow("x as multiplication sign") << "25x4" << "100";
    QTest::newRow("single digit factorial") << "5!" << "120";
    QTest::newRow("superscripted number") << "2³"
                                          << "8"; // BUG: 435932

    QTest::newRow("hex to decimal lower case") << "0xf" << "15";
    QTest::newRow("hex to decimal upper case") << "0xF" << "15";
    QTest::newRow("hex to decimal with = at beginning") << "=0xF" << "15";
    QTest::newRow("decimal to hex") << "hex=15" << "0xF";
    QTest::newRow("decimal to hex with addition") << "hex=15+15" << "0x1E";
    QTest::newRow("hex additions") << "0xF+0xF" << "30";
    QTest::newRow("hex multiplication") << "0xF*0xF" << "225";
    // BUG: 431362
    QTest::newRow("hex and decimal addition") << "0x12+1" << "19";
    QTest::newRow("hex and decimal addition reversed") << "1+0x12" << "19";
    // clang-format on
}

void CalculatorRunnerTest::testApproximation()
{
    const auto matches = launchQuery("5^1234567");
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.first().subtext(), "Approximation");
}

void CalculatorRunnerTest::test42()
{
    auto matches = launchQuery("life");
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.constFirst().text(), "42");
    matches = launchQuery("universe");
    QCOMPARE(matches.size(), 1);
    QCOMPARE(matches.constFirst().text(), "42");
}

#if QALCULATE_MAJOR_VERSION > 2 || QALCULATE_MINOR_VERSION > 6
void CalculatorRunnerTest::testErrorDetection()
{
    launchQuery("SDL_VIDEODRIVER=");
    QVERIFY(manager->matches().isEmpty());
}
#endif

void CalculatorRunnerTest::testFunctions()
{
    // BUG: 467418
    launchQuery("sqrt(4)");
    QCOMPARE(manager->matches().size(), 1);

    launchQuery("=sqrt(4)");
    QCOMPARE(manager->matches().size(), 1);

    // Goes to qalculate
    launchQuery("=sqrt 4");
    QCOMPARE(manager->matches().size(), 1);

    // Does not match the prefixless queries and function-pattern
    launchQuery("sqrt 4");
    QCOMPARE(manager->matches().size(), 0);
}

QTEST_MAIN(CalculatorRunnerTest)

#include "calculatorrunnertest.moc"
