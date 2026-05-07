/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>
    SPDX-FileCopyrightText: 2021 Alexander Lohnau <alexander.lohnau@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "testchromebookmarks.h"
#include "browsers/chrome.h"
#include "browsers/chromefindprofile.h"
#include "favicon.h"
#include <QTest>

using namespace Qt::StringLiterals;
using namespace KRunner;

void verifyMatch(BookmarkMatch &match, const QString &title, const QString &url)
{
    QCOMPARE(match.bookmarkTitle(), title);
    QCOMPARE(match.bookmarkUrl(), url);
}

void TestChromeBookmarks::initTestCase()
{
    m_configHome = QFINDTESTDATA("chrome-config-home");
}

void TestChromeBookmarks::bookmarkFinderShouldFindEachProfileDirectory()
{
    FindChromeProfile findChrome(u"chromium"_s, m_configHome);
    QString profileTemplate = m_configHome + u"/.config/%1/%2/Bookmarks";

    QList<Profile> profiles = findChrome.find();
    QCOMPARE(profiles.size(), 2);
    QCOMPARE(profiles[0].path(), profileTemplate.arg(u"chromium", u"Default"));
    QCOMPARE(profiles[1].path(), profileTemplate.arg(u"chromium", u"Profile 1"));
}

void TestChromeBookmarks::bookmarkFinderShouldReportNoProfilesOnErrors()
{
    FindChromeProfile findChrome(u"chromium"_s, u"./non-existent-directory"_s);
    QList<Profile> profiles = findChrome.find();
    QCOMPARE(profiles.size(), 0);
}

void TestChromeBookmarks::itShouldFindNothingWhenPrepareIsNotCalled()
{
    auto finder = std::make_unique<FakeFindProfile>();
    finder->addProfile(m_configHome + u"/Chrome-Bookmarks-Sample.json", u"Sample"_s, std::make_unique<FallbackFavicon>());

    Chrome chrome(std::move(finder));
    QList<BookmarkMatch> matches = chrome.match(u"any"_s, true);
    QCOMPARE(chrome.match(u"any"_s, true).size(), 0);
}

void TestChromeBookmarks::itShouldGracefullyExitWhenFileIsNotFound()
{
    auto finder = std::make_unique<FakeFindProfile>();
    finder->addProfile(u"non-existent-file.json"_s, u"Sample"_s, std::make_unique<FallbackFavicon>());

    Chrome chrome(std::move(finder));
    chrome.prepare();
    QList<BookmarkMatch> matches = chrome.match(u"any"_s, true);
    QCOMPARE(chrome.match(u"any"_s, true).size(), 0);
}

void TestChromeBookmarks::itShouldFindAllBookmarks()
{
    auto finder = std::make_unique<FakeFindProfile>();
    finder->addProfile(m_configHome + u"/Chrome-Bookmarks-Sample.json", u"Sample"_s, std::make_unique<FallbackFavicon>());

    Chrome chrome(std::move(finder));
    chrome.prepare();

    // The sample JSON contains 3 bookmarks
    QList<BookmarkMatch> matches = chrome.match(u"any"_s, true);
    QCOMPARE(matches.size(), 3);
    verifyMatch(matches[0], u"some bookmark in bookmark bar"_s, u"https://somehost.com/"_s);
    verifyMatch(matches[1], u"bookmark in other bookmarks"_s, u"https://otherbookmarks.com/"_s);
    verifyMatch(matches[2], u"bookmark in somefolder"_s, u"https://somefolder.com/"_s);
}

void TestChromeBookmarks::itShouldFindOnlyMatches()
{
    auto finder = std::make_unique<FakeFindProfile>();
    finder->addProfile(m_configHome + u"/Chrome-Bookmarks-Sample.json", u"Sample"_s, std::make_unique<FallbackFavicon>());

    Chrome chrome(std::move(finder));
    chrome.prepare();

    // Testing match for specific term "other" from the sample JSON
    QList<BookmarkMatch> matches = chrome.match(u"other"_s, false);
    QCOMPARE(matches.size(), 1);
    verifyMatch(matches[0], u"bookmark in other bookmarks"_s, u"https://otherbookmarks.com/"_s);
}

void TestChromeBookmarks::itShouldClearResultAfterCallingTeardown()
{
    auto finder = std::make_unique<FakeFindProfile>();
    finder->addProfile(m_configHome + u"/Chrome-Bookmarks-Sample.json", u"Sample"_s, std::make_unique<FallbackFavicon>());

    Chrome chrome(std::move(finder));
    chrome.prepare();
    QCOMPARE(chrome.match(u"any"_s, true).size(), 3);

    chrome.teardown();
    QCOMPARE(chrome.match(u"any"_s, true).size(), 0);
}

void TestChromeBookmarks::itShouldFindBookmarksFromAllProfiles()
{
    auto finder = std::make_unique<FakeFindProfile>();
    finder->addProfile(m_configHome + u"/Chrome-Bookmarks-Sample.json", u"Sample"_s, std::make_unique<FallbackFavicon>());
    finder->addProfile(m_configHome + u"/Chrome-Bookmarks-SecondProfile.json", u"SecondProfile"_s, std::make_unique<FallbackFavicon>());

    Chrome chrome(std::move(finder));
    chrome.prepare();

    // Combined profiles contain 4 bookmarks
    QList<BookmarkMatch> matches = chrome.match(u"any"_s, true);
    QCOMPARE(matches.size(), 4);
    verifyMatch(matches[0], u"some bookmark in bookmark bar"_s, u"https://somehost.com/"_s);
    verifyMatch(matches[1], u"bookmark in other bookmarks"_s, u"https://otherbookmarks.com/"_s);
    verifyMatch(matches[2], u"bookmark in somefolder"_s, u"https://somefolder.com/"_s);
    verifyMatch(matches[3], u"bookmark in secondProfile"_s, u"https://secondprofile.com/"_s);
}

QTEST_MAIN(TestChromeBookmarks)

#include "testchromebookmarks.moc"