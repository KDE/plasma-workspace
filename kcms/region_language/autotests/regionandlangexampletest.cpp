/*
    regionandlangexampletest.cpp
    SPDX-FileCopyrightText: 2022 Han Young <hanyoung@protonmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QtTest>
#include <exampleutility.h>

class ExampleUtilityTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testUnicodeSymbolReplacing();
};

void ExampleUtilityTest::testUnicodeSymbolReplacing()
{
#ifdef LC_ADDRESS
    static const auto case1 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("T<U00FC>rkiye"));
    static const auto case1Result = QStringLiteral("Türkiye");

    static const auto case2 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("<U00C7>ar<U015F>amba"));
    static const auto case2Result = QStringLiteral("Çarşamba");

    static const auto case3 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("<U0423><U043A><U0440><U0430><U0457><U043D><U0430>"));
    static const auto case3Result = QStringLiteral("Україна");

    static const auto case4 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("<U4E2D><U83EF><U6C11><U570B>"));
    static const auto case4Result = QStringLiteral("中華民國");

    static const auto case5 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("<U4E2D><U83EF><-><U6C11><U570B>"));
    static const auto case5Result = QStringLiteral("中華<->民國");

    static const auto case6 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("<U4E2D><U83EF><U77777777><U6C11><U570B>"));
    static const auto case6Result = QStringLiteral("中華<U77777777>民國");

    static const auto case7 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("<U4E2D><U83EF><U6C11><U570B><U"));
    static const auto case7Result = QStringLiteral("中華民國<U");

    static const auto case8 = Utility::replaceASCIIUnicodeSymbol(QStringLiteral("Plain ASCII STRING"));
    static const auto case8Result = QStringLiteral("Plain ASCII STRING");

    QCOMPARE(case1, case1Result);
    QCOMPARE(case2, case2Result);
    QCOMPARE(case3, case3Result);
    QCOMPARE(case4, case4Result);
    QCOMPARE(case5, case5Result);
    QCOMPARE(case6, case6Result);
    QCOMPARE(case7, case7Result);
    QCOMPARE(case8, case8Result);
    QCOMPARE(Utility::replaceASCIIUnicodeSymbol(QLatin1String("")), QLatin1String(""));
#endif
}
QTEST_MAIN(ExampleUtilityTest)
#include "regionandlangexampletest.moc"
