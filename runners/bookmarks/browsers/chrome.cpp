/*
 *   Copyright 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
 *   Copyright 2012 Glenn Ergeerts <marco.gulino@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include "chrome.h"
#include "faviconfromblob.h"
#include "browsers/findprofile.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QDebug>
#include "bookmarksrunner_defs.h"
#include <QFileInfo>
#include <QDir>

class ProfileBookmarks {
public:
    ProfileBookmarks(Profile &profile) : m_profile(profile) {}
    inline QList<QVariantMap> bookmarks() { return m_bookmarks; }
    inline Profile profile() { return m_profile; }
    void tearDown() { m_profile.favicon()->teardown(); m_bookmarks.clear(); }
    void add(QVariantMap &bookmarkEntry) { m_bookmarks << bookmarkEntry; }
    void clear() { m_bookmarks.clear(); }
private:
    Profile m_profile;
    QList<QVariantMap> m_bookmarks;
};

Chrome::Chrome( FindProfile* findProfile, QObject* parent )
    : QObject(parent),
    m_watcher(new KDirWatch(this)),
    m_dirty(false)
{
    foreach(Profile profile, findProfile->find()) {
        m_profileBookmarks << new ProfileBookmarks(profile);
        m_watcher->addFile(profile.path());
    }
    connect(m_watcher, &KDirWatch::created, [=] { m_dirty = true; });
}

Chrome::~Chrome()
{
    foreach(ProfileBookmarks *profileBookmark, m_profileBookmarks) {
        delete profileBookmark;
    }
}

QList<BookmarkMatch> Chrome::match(const QString &term, bool addEveryThing)
{
    if (m_dirty) {
        prepare();
    }
    QList<BookmarkMatch> results;
    foreach(ProfileBookmarks *profileBookmarks, m_profileBookmarks) {
        results << match(term, addEveryThing, profileBookmarks);
    }
    return results;
}

QList<BookmarkMatch> Chrome::match(const QString &term, bool addEveryThing, ProfileBookmarks *profileBookmarks)
{
    QList<BookmarkMatch> results;
    foreach(const QVariantMap &bookmark, profileBookmarks->bookmarks()) {
        QString url = bookmark.value(QStringLiteral("url")).toString();

        BookmarkMatch bookmarkMatch(profileBookmarks->profile().favicon(), term, bookmark.value(QStringLiteral("name")).toString(), url);
        bookmarkMatch.addTo(results, addEveryThing);
    }
    return results;
}

void Chrome::prepare()
{
    m_dirty = false;
    foreach(ProfileBookmarks *profileBookmarks, m_profileBookmarks) {
        Profile profile = profileBookmarks->profile();
        profileBookmarks->clear();
        QFile bookmarksFile(profile.path());
        if (!bookmarksFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        };
        QJsonDocument jdoc = QJsonDocument::fromJson(bookmarksFile.readAll());
        if (jdoc.isNull()) {
            continue;
        }
        const QVariantMap resultMap = jdoc.object().toVariantMap();
        if (!resultMap.contains(QStringLiteral("roots"))) {
            return;
        }
        const QVariantMap entries = resultMap.value(QStringLiteral("roots")).toMap();
        foreach(const QVariant &folder, entries) {
            parseFolder(folder.toMap(), profileBookmarks);
        }
        profile.favicon()->prepare();
    }
}

void Chrome::teardown()
{
    foreach(ProfileBookmarks *profileBookmarks, m_profileBookmarks) {
        profileBookmarks->tearDown();
    }
}

void Chrome::parseFolder(const QVariantMap &entry, ProfileBookmarks *profile)
{
    QVariantList children = entry.value(QStringLiteral("children")).toList();
    foreach(const QVariant &child, children) {
        QVariantMap entry = child.toMap();
        if(entry.value(QStringLiteral("type")).toString() == QLatin1String("folder"))
            parseFolder(entry, profile);
        else {
            profile->add(entry);
        }
    }
}
