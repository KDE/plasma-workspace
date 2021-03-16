#include <QObject>
#include <QStandardPaths>
#include <QTest>

#include "browsers/firefox.h"

class TestFirefoxBookmarks : public QObject
{
    Q_OBJECT
public:
    explicit TestFirefoxBookmarks(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

private:
    Firefox *m_firefox;

private Q_SLOTS:
    void initTestCase();
    void testAllBookmarks();
    void testBookmarksQuery();
    void testBookmarksQuery_data();
};

void TestFirefoxBookmarks::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    m_firefox = new Firefox(this, QFINDTESTDATA("firefox-config-home"));
}

void TestFirefoxBookmarks::testAllBookmarks()
{
    m_firefox->prepare();
    const QList<BookmarkMatch> matches = m_firefox->match(QStringLiteral("kde"), true);
    QCOMPARE(matches.count(), 8);
    m_firefox->teardown();
}

void TestFirefoxBookmarks::testBookmarksQuery_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<QStringList>("expectedUrls");

    QTest::newRow("query that matches nothing") << "this does not exist" << QStringList{};
    QTest::newRow("query that matches url") << "reddit.com" << QStringList{"https://www.reddit.com/"};
    QTest::newRow("query that matches title") << "KDE Community" << QStringList{"https://kde.org/"};
    QTest::newRow("query that matches title case insensitively") << "kde community" << QStringList{"https://kde.org/"};
    QTest::newRow("query that matches multiple titles") << "ubuntu" << QStringList{"http://www.ubuntu.com/", "http://wiki.ubuntu.com/"};
}

void TestFirefoxBookmarks::testBookmarksQuery()
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

QTEST_MAIN(TestFirefoxBookmarks)

#include "testfirefoxbookmarks.moc"
