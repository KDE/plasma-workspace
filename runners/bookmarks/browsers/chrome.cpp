/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "chrome.h"
#include "browsers/findprofile.h"
#include "faviconfromblob.h"

#include <QDebug>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

class ProfileBookmarks
{
public:
    ProfileBookmarks(const Profile &profile)
        : m_profile(profile)
    {
    }
    inline QJsonArray bookmarks()
    {
        return m_bookmarks;
    }
    inline Profile profile()
    {
        return m_profile;
    }
    void tearDown()
    {
        m_profile.favicon()->teardown();
        clear();
    }
    void add(const QJsonObject &bookmarkEntry)
    {
        m_bookmarks << bookmarkEntry;
    }
    void add(const QJsonArray &entries)
    {
        for (const auto &e : entries)
            m_bookmarks << e;
    }
    void clear()
    {
        m_bookmarks = QJsonArray();
    }

private:
    Profile m_profile;
    QJsonArray m_bookmarks;
};

Chrome::Chrome(FindProfile *findProfile, QObject *parent)
    : QObject(parent)
    , m_watcher(new KDirWatch(this))
    , m_dirty(false)
{
    const auto profiles = findProfile->find();
    for (const Profile &profile : profiles) {
        updateCacheFile(profile.faviconSource(), profile.faviconCache());
        m_profileBookmarks << new ProfileBookmarks(profile);
        m_watcher->addFile(profile.path());
    }
    connect(m_watcher, &KDirWatch::created, this, [this] {
        m_dirty = true;
    });
}

Chrome::~Chrome()
{
    for (ProfileBookmarks *profileBookmark : qAsConst(m_profileBookmarks)) {
        delete profileBookmark;
    }
}

QList<BookmarkMatch> Chrome::match(const QString &term, bool addEveryThing)
{
    if (m_dirty) {
        prepare();
    }
    QList<BookmarkMatch> results;
    for (ProfileBookmarks *profileBookmarks : qAsConst(m_profileBookmarks)) {
        results << match(term, addEveryThing, profileBookmarks);
    }
    return results;
}

QList<BookmarkMatch> Chrome::match(const QString &term, bool addEveryThing, ProfileBookmarks *profileBookmarks)
{
    QList<BookmarkMatch> results;

    const auto bookmarks = profileBookmarks->bookmarks();
    Favicon *favicon = profileBookmarks->profile().favicon();
    for (const QJsonValue &bookmarkValue : bookmarks) {
        const QJsonObject bookmark = bookmarkValue.toObject();
        const QString url = bookmark.value(QStringLiteral("url")).toString();
        BookmarkMatch bookmarkMatch(favicon->iconFor(url), term, bookmark.value(QStringLiteral("name")).toString(), url);
        bookmarkMatch.addTo(results, addEveryThing);
    }
    return results;
}

void Chrome::prepare()
{
    m_dirty = false;
    for (ProfileBookmarks *profileBookmarks : qAsConst(m_profileBookmarks)) {
        Profile profile = profileBookmarks->profile();
        profileBookmarks->clear();
        const QJsonArray bookmarks = readChromeFormatBookmarks(profile.path());
        if (bookmarks.isEmpty()) {
            continue;
        }
        profileBookmarks->add(bookmarks);
        updateCacheFile(profile.faviconSource(), profile.faviconCache());
        profile.favicon()->prepare();
    }
}

void Chrome::teardown()
{
    for (ProfileBookmarks *profileBookmarks : qAsConst(m_profileBookmarks)) {
        profileBookmarks->tearDown();
    }
}
