/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QObject>
#include <QStandardPaths>
#include <QTest>

#include "browsers/firefox.h"

using namespace Qt::StringLiterals;
using namespace KRunner;

class TestBookmarksMatch : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

private Q_SLOTS:
    void testQueryMatchConversion();
    void testQueryMatchConversion_data();
    void testAddToList();
};

void TestBookmarksMatch::testQueryMatchConversion()
{
    QFETCH(QString, searchTerm);
    QFETCH(QString, bookmarkDescription);
    QFETCH(int, expectedMatchCategoryRelevance);
    QFETCH(qreal, expectedRelevance);

    BookmarkMatch bookmarkMatch(QIcon::fromTheme(u"unknown"_s), searchTerm, u"KDE Community"_s, u"https://somehost.com/"_s, bookmarkDescription);
    QueryMatch match = bookmarkMatch.asQueryMatch(nullptr);

    QCOMPARE(match.text(), u"KDE Community");
    QCOMPARE(match.data().toString(), u"https://somehost.com/");
    QCOMPARE(match.categoryRelevance(), expectedMatchCategoryRelevance);
    QCOMPARE(match.relevance(), expectedRelevance);
}

void TestBookmarksMatch::testQueryMatchConversion_data()
{
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<QString>("bookmarkDescription");
    QTest::addColumn<int>("expectedMatchCategoryRelevance");
    QTest::addColumn<qreal>("expectedRelevance");

    auto newRow = [](const char *dataTag, const QString searchTerm, const QString bookmarkDescription, int expectedMatchType, qreal expectedRelevance) {
        QTest::newRow(dataTag) << searchTerm << bookmarkDescription << expectedMatchType << expectedRelevance;
    };

    newRow("no text match", u"krunner"_s, QString(), (int)QueryMatch::CategoryRelevance::Low, 0.18);
    newRow("title partly matches", u"kde"_s, QString(), (int)QueryMatch::CategoryRelevance::Low, 0.45);
    newRow("title exactly matches", u"kde community"_s, QString(), (int)QueryMatch::CategoryRelevance::Highest, 1.0);
    newRow("url partly matches", u"somehost"_s, QString(), (int)QueryMatch::CategoryRelevance::Low, 0.2);
    newRow("url exactly matches", u"https://somehost.com/"_s, QString(), (int)QueryMatch::CategoryRelevance::Low, 0.2);
    newRow("description exactly matches", u"test"_s, u"test"_s, (int)QueryMatch::CategoryRelevance::Highest, 1.0);
    newRow("description partly matches", u"test"_s, u"testme"_s, (int)QueryMatch::CategoryRelevance::Low, 0.3);
}

void TestBookmarksMatch::testAddToList()
{
    BookmarkMatch noMatch(QIcon(), u"krunner"_s, u"KDE Community"_s, u"https://somehost.com/"_s);
    BookmarkMatch match(QIcon(), u"kde"_s, u"KDE Community"_s, u"https://somehost.com/"_s);

    QList<BookmarkMatch> onlyMatching;
    noMatch.addTo(onlyMatching, false);
    match.addTo(onlyMatching, false);
    QCOMPARE(onlyMatching.count(), 1);

    QList<BookmarkMatch> allMatches;
    noMatch.addTo(allMatches, true);
    match.addTo(allMatches, true);
    QCOMPARE(allMatches.count(), 2);
}

QTEST_MAIN(TestBookmarksMatch)

#include "bookmarksmatchtest.moc"
