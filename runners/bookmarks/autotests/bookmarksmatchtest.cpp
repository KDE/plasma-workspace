/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QObject>
#include <QStandardPaths>
#include <QTest>

#include "browsers/firefox.h"

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
    QFETCH(int, expectedMatchType);
    QFETCH(qreal, expectedRelevance);

    BookmarkMatch bookmarkMatch(QIcon::fromTheme("unknown"), searchTerm, "KDE Community", "https://somehost.com/", bookmarkDescription);
    QueryMatch match = bookmarkMatch.asQueryMatch(nullptr);

    QCOMPARE(match.text(), "KDE Community");
    QCOMPARE(match.data().toString(), "https://somehost.com/");
    QCOMPARE(match.categoryRelevance(), expectedMatchType);
    QCOMPARE(match.relevance(), expectedRelevance);
}

void TestBookmarksMatch::testQueryMatchConversion_data()
{
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<QString>("bookmarkDescription");
    QTest::addColumn<int>("expectedMatchType");
    QTest::addColumn<qreal>("expectedRelevance");

    auto newRow = [](const char *dataTag,
                     const QString searchTerm,
                     const QString bookmarkDescription,
                     KRunner::QueryMatch::CategoryRelevance expectedMatchType,
                     qreal expectedRelevance) {
        QTest::newRow(dataTag) << searchTerm << bookmarkDescription << (int)expectedMatchType << expectedRelevance;
    };

    newRow("no text match", "krunner", "", KRunner::QueryMatch::CategoryRelevance::Low, 0.18);
    newRow("title partly matches", "kde", "", KRunner::QueryMatch::CategoryRelevance::Low, 0.45);
    newRow("title exactly matches", "kde community", "", KRunner::QueryMatch::CategoryRelevance::Highest, 1.0);
    newRow("url partly matches", "somehost", "", KRunner::QueryMatch::CategoryRelevance::Low, 0.2);
    newRow("url exactly matches", "https://somehost.com/", "", KRunner::QueryMatch::CategoryRelevance::Low, 0.2);
    newRow("description exactly matches", "test", "test", KRunner::QueryMatch::CategoryRelevance::Highest, 1.0);
    newRow("description partly matches", "test", "testme", KRunner::QueryMatch::CategoryRelevance::Low, 0.3);
}

void TestBookmarksMatch::testAddToList()
{
    BookmarkMatch noMatch(QIcon(), "krunner", "KDE Community", "https://somehost.com/");
    BookmarkMatch match(QIcon(), "kde", "KDE Community", "https://somehost.com/");

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
