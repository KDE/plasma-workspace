/*
    SPDX-FileCopyrightText: 2022 Popov Eugene <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../utils.h"

#include <QObject>
#include <QtTest>

class UtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSimplifiedText();
};

void UtilsTest::testSimplifiedText()
{
    const QString text = QStringLiteral("   Some    text\n   to\t\ttest    ");

    QCOMPARE(Utils::simplifiedText(text, 1000), QStringLiteral("Some text to test"));
    QCOMPARE(Utils::simplifiedText(text, 17), QStringLiteral("Some text to test"));
    QCOMPARE(Utils::simplifiedText(text, 10), QStringLiteral("Some text"));
    QCOMPARE(Utils::simplifiedText(text, 0), QStringLiteral(""));
}

QTEST_MAIN(UtilsTest)
#include "utilstest.moc"
