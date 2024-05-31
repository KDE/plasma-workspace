/*
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QObject>
#include <QStandardPaths>
#include <QTest>

#include "browsers/firefox.h"

using namespace Qt::StringLiterals;

class TestFirefoxBookmarksMatch : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

private:
    Firefox *m_firefox = nullptr;

private Q_SLOTS:
    void initTestCase();
    void testAllBookmarks();
    void testBookmarksQuery();
    void testBookmarksQuery_data();
};

void TestFirefoxBookmarksMatch::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    m_firefox = new Firefox(QFINDTESTDATA("firefox-config-home"), this);
}

void TestFirefoxBookmarksMatch::testAllBookmarks()
{
    m_firefox->prepare();
    const QList<BookmarkMatch> matches = m_firefox->match(QStringLiteral("kde"), true);
    QCOMPARE(matches.count(), 8);
    m_firefox->teardown();
}

void TestFirefoxBookmarksMatch::testBookmarksQuery_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QStringList>("expectedUrls");

    QTest::newRow("query that matches nothing") << u"this does not exist"_s << QStringList{};
    QTest::newRow("query that matches url") << u"reddit.com"_s << QStringList{u"https://www.reddit.com/"_s};
    QTest::newRow("query that matches title") << u"KDE Community"_s << QStringList{u"https://kde.org/"_s};
    QTest::newRow("query that matches title case insensitively") << u"kde community"_s << QStringList{u"https://kde.org/"_s};
    QTest::newRow("query that matches multiple titles") << u"ubuntu"_s << QStringList{u"http://www.ubuntu.com/"_s, u"http://wiki.ubuntu.com/"_s};
}

void TestFirefoxBookmarksMatch::testBookmarksQuery()
{
    QFETCH(QString, query);
    QFETCH(QStringList, expectedUrls);

    m_firefox->prepare();
    const QList<BookmarkMatch> matches = m_firefox->match(query, false);
    QCOMPARE(matches.count(), expectedUrls.count());
    for (const auto &match : matches) {
        QVERIFY(expectedUrls.contains(match.bookmarkUrl()));
    }
    m_firefox->teardown();
}

QTEST_MAIN(TestFirefoxBookmarksMatch)

#include "testfirefoxbookmarks.moc"
