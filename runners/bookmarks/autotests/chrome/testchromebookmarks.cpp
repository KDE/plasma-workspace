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

using namespace Plasma;

void TestChromeBookmarks::initTestCase()
{
    m_configHome = QFINDTESTDATA("chrome-config-home");
    m_findBookmarksInCurrentDirectory.reset(
        new FakeFindProfile(QList<Profile>({Profile(m_configHome + "/Chrome-Bookmarks-Sample.json", "Sample", new FallbackFavicon())})));
}

void TestChromeBookmarks::bookmarkFinderShouldFindEachProfileDirectory()
{
    FindChromeProfile findChrome("chromium", m_configHome);
    QString profileTemplate = m_configHome + "/.config/%1/%2/Bookmarks";

    QList<Profile> profiles = findChrome.find();
    QCOMPARE(profiles.size(), 2);
    QCOMPARE(profiles[0].path(), profileTemplate.arg("chromium", "Default"));
    QCOMPARE(profiles[1].path(), profileTemplate.arg("chromium", "Profile 1"));
}

void TestChromeBookmarks::bookmarkFinderShouldReportNoProfilesOnErrors()
{
    FindChromeProfile findChrome("chromium", "./no-config-directory");
    QList<Profile> profiles = findChrome.find();
    QCOMPARE(profiles.size(), 0);
}

void TestChromeBookmarks::itShouldFindNothingWhenPrepareIsNotCalled()
{
    Chrome *chrome = new Chrome(m_findBookmarksInCurrentDirectory.get(), this);
    QCOMPARE(chrome->match("any", true).size(), 0);
}

void TestChromeBookmarks::itShouldGracefullyExitWhenFileIsNotFound()
{
    FakeFindProfile finder(QList<Profile>() << Profile("FileNotExisting.json", QString(), nullptr));
    Chrome *chrome = new Chrome(&finder, this);
    chrome->prepare();
    QCOMPARE(chrome->match("any", true).size(), 0);
}

void verifyMatch(BookmarkMatch &match, const QString &title, const QString &url)
{
    QCOMPARE(match.bookmarkTitle(), title);
    QCOMPARE(match.bookmarkUrl(), url);
}

void TestChromeBookmarks::itShouldFindAllBookmarks()
{
    Chrome *chrome = new Chrome(m_findBookmarksInCurrentDirectory.get(), this);
    chrome->prepare();
    QList<BookmarkMatch> matches = chrome->match("any", true);
    QCOMPARE(matches.size(), 3);
    verifyMatch(matches[0], "some bookmark in bookmark bar", "https://somehost.com/");
    verifyMatch(matches[1], "bookmark in other bookmarks", "https://otherbookmarks.com/");
    verifyMatch(matches[2], "bookmark in somefolder", "https://somefolder.com/");
}

void TestChromeBookmarks::itShouldFindOnlyMatches()
{
    Chrome *chrome = new Chrome(m_findBookmarksInCurrentDirectory.get(), this);
    chrome->prepare();
    QList<BookmarkMatch> matches = chrome->match("other", false);
    QCOMPARE(matches.size(), 1);
    verifyMatch(matches[0], "bookmark in other bookmarks", "https://otherbookmarks.com/");
}

void TestChromeBookmarks::itShouldClearResultAfterCallingTeardown()
{
    Chrome *chrome = new Chrome(m_findBookmarksInCurrentDirectory.get(), this);
    chrome->prepare();
    QCOMPARE(chrome->match("any", true).size(), 3);
    chrome->teardown();
    QCOMPARE(chrome->match("any", true).size(), 0);
}

void TestChromeBookmarks::itShouldFindBookmarksFromAllProfiles()
{
    FakeFindProfile findBookmarksFromAllProfiles(
        QList<Profile>{Profile(m_configHome + "/Chrome-Bookmarks-Sample.json", "Sample", new FallbackFavicon(this)),
                       Profile(m_configHome + "/Chrome-Bookmarks-SecondProfile.json", "SecondProfile", new FallbackFavicon(this))});
    Chrome *chrome = new Chrome(&findBookmarksFromAllProfiles, this);
    chrome->prepare();
    QList<BookmarkMatch> matches = chrome->match("any", true);
    QCOMPARE(matches.size(), 4);
    verifyMatch(matches[0], "some bookmark in bookmark bar", "https://somehost.com/");
    verifyMatch(matches[1], "bookmark in other bookmarks", "https://otherbookmarks.com/");
    verifyMatch(matches[2], "bookmark in somefolder", "https://somefolder.com/");
    verifyMatch(matches[3], "bookmark in secondProfile", "https://secondprofile.com/");
}

QTEST_MAIN(TestChromeBookmarks);
