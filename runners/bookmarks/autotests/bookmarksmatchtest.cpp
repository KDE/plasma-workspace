/**
 * SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QObject>
#include <QStandardPaths>
#include <QTest>

#include "browsers/firefox.h"

using namespace Plasma;
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
    QFETCH(QString, bookmarkTitle);
    QFETCH(QString, bookmarkUrl);
    QFETCH(QString, bookmarkDescription);
    QFETCH(int, expectedMatchType);
    QFETCH(qreal, expectedRelevance);

    BookmarkMatch bookmarkMatch(QIcon::fromTheme("unknown"), searchTerm, bookmarkTitle, bookmarkUrl, bookmarkDescription);
    QueryMatch match = bookmarkMatch.asQueryMatch(nullptr);

    QCOMPARE(match.text(), bookmarkTitle);
    QCOMPARE(match.data().toString(), bookmarkUrl);
    QCOMPARE(match.type(), expectedMatchType);
    QCOMPARE(match.relevance(), expectedRelevance);
}

void TestBookmarksMatch::testQueryMatchConversion_data()
{
    QTest::addColumn<QString>("searchTerm");
    QTest::addColumn<QString>("bookmarkTitle");
    QTest::addColumn<QString>("bookmarkUrl");
    QTest::addColumn<QString>("bookmarkDescription");
    QTest::addColumn<int>("expectedMatchType");
    QTest::addColumn<qreal>("expectedRelevance");

    QTest::newRow("no text match") << "krunner"
                                   << "KDE Community"
                                   << "https://somehost.com/"
                                   << "" << (int)QueryMatch::PossibleMatch << 0.18;
    QTest::newRow("title partly matches") << "kde"
                                          << "KDE Community"
                                          << "https://somehost.com/"
                                          << "" << (int)QueryMatch::PossibleMatch << 0.45;
    QTest::newRow("title exactly matches") << "kde community"
                                           << "KDE Community"
                                           << "https://somehost.com/"
                                           << "" << (int)QueryMatch::ExactMatch << 1.0;
    QTest::newRow("url partly matches") << "somehost"
                                        << "KDE Community"
                                        << "https://somehost.com/"
                                        << "" << (int)QueryMatch::PossibleMatch << 0.2;
    QTest::newRow("url exactly matches") << "https://somehost.com/"
                                         << "KDE Community"
                                         << "https://somehost.com/"
                                         << "" << (int)QueryMatch::PossibleMatch << 0.2;
    QTest::newRow("description exactly matches") << "test"
                                                 << "KDE Community"
                                                 << "https://somehost.com/"
                                                 << "test" << (int)QueryMatch::ExactMatch << 1.0;
    QTest::newRow("description partly matches") << "test"
                                                << "KDE Community"
                                                << "https://somehost.com/"
                                                << "testme" << (int)QueryMatch::PossibleMatch << 0.3;
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
