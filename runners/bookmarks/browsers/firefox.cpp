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

#include "firefox.h"
#include <KJob>
#include <QDebug>
#include "bookmarksrunner_defs.h"
#include <QFile>
#include <QDir>
#include <KConfigGroup>
#include <KSharedConfig>
#include "bookmarkmatch.h"
#include "favicon.h"
#include "fetchsqlite.h"
#include "faviconfromblob.h"

Firefox::Firefox(QObject *parent) :
    QObject(parent),
    m_favicon(new FallbackFavicon(this)),
    m_fetchsqlite(nullptr),
    m_fetchsqlite_fav(nullptr)
{
  reloadConfiguration();
  //qDebug() << "Loading Firefox Bookmarks Browser";
}


Firefox::~Firefox()
{
    if (!m_dbCacheFile.isEmpty()) {
        QFile db_CacheFile(m_dbCacheFile);
        if (db_CacheFile.exists()) {
            //qDebug() << "Cache file was removed: " << db_CacheFile.remove();
        }
    }
    //qDebug() << "Deleted Firefox Bookmarks Browser";
}

void Firefox::prepare()
{
    if (m_dbCacheFile.isEmpty()) {
        m_dbCacheFile = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/bookmarkrunnerfirefoxdbfile.sqlite");
    }
    if (m_dbCacheFile_fav.isEmpty()) {
        m_dbCacheFile_fav = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/bookmarkrunnerfirefoxfavdbfile.sqlite");
    }
    if (!m_dbFile.isEmpty()) {
        m_fetchsqlite = new FetchSqlite(m_dbFile, m_dbCacheFile);
        m_fetchsqlite->prepare();
    }
    if (!m_dbFile_fav.isEmpty()) {
        m_fetchsqlite_fav = new FetchSqlite(m_dbFile_fav, m_dbCacheFile_fav);
        m_fetchsqlite_fav->prepare();

        delete m_favicon;
        m_favicon = nullptr;

        m_favicon = FaviconFromBlob::firefox(m_fetchsqlite_fav, this);
    }
}

QList< BookmarkMatch > Firefox::match(const QString& term, bool addEverything)
{
    QList< BookmarkMatch > matches;
    if (!m_fetchsqlite) {
        return matches;
    }
    //qDebug() << "Firefox bookmark: match " << term;

    QString tmpTerm = term;
    QString query;
    if (addEverything) {
        query = QStringLiteral("SELECT moz_bookmarks.fk, moz_bookmarks.title, moz_places.url " \
                    "FROM moz_bookmarks, moz_places WHERE " \
                    "moz_bookmarks.type = 1 AND moz_bookmarks.fk = moz_places.id");
    } else {
        const QString escapedTerm = tmpTerm.replace('\'', QLatin1String("\\'"));
        query = QString("SELECT moz_bookmarks.fk, moz_bookmarks.title, moz_places.url " \
                        "FROM moz_bookmarks, moz_places WHERE " \
                        "moz_bookmarks.type = 1 AND moz_bookmarks.fk = moz_places.id AND " \
                        "(moz_bookmarks.title LIKE  '%" + escapedTerm + "%' or moz_places.url LIKE '%"
                        + escapedTerm + "%')");
    }
    QList<QVariantMap> results = m_fetchsqlite->query(query, QMap<QString, QVariant>());
    QMultiMap<QString, QString> uniqueResults;
    foreach(QVariantMap result, results) {
        const QString title = result.value(QStringLiteral("title")).toString();
        const QUrl url = result.value(QStringLiteral("url")).toUrl();
        if (url.isEmpty() || url.scheme() == QLatin1String("place")) {
            // Don't use bookmarks with empty url or Firefox's "place:" scheme,
            // e.g. used for "Most Visited" or "Recent Tags"
            //qDebug() << "element " << url << " was not added";
            continue;
        }

        auto urlString = url.toString();
        // After joining we may have multiple results for each URL:
        // 1) one for each bookmark folder (same or different titles)
        // 2) one for each tag (no title for all but the first entry)
        auto keyRange = uniqueResults.equal_range(urlString);
        auto it = keyRange.first;
        if (!title.isEmpty()) {
            while (it != keyRange.second) {
                if (*it == title) {
                    // same URL and title in multiple bookmark folders
                    break;
                }
                if (it->isEmpty()) {
                    // add a title if there was none for the URL
                    *it = title;
                    break;
                }
                ++it;
            }
        }
        if (it == keyRange.second) {
            // first or unique entry
            uniqueResults.insert(urlString, title);
        }
    }

    for (auto result = uniqueResults.constKeyValueBegin(); result != uniqueResults.constKeyValueEnd(); ++result) {
        BookmarkMatch bookmarkMatch(m_favicon, term, (*result).second, (*result).first);
        bookmarkMatch.addTo(matches, addEverything);
    }

    return matches;
}


void Firefox::teardown()
{
    if(m_fetchsqlite) {
        m_fetchsqlite->teardown();
    }
    if(m_fetchsqlite_fav) {
        m_fetchsqlite_fav->teardown();
        delete m_favicon;
        m_favicon = nullptr;
    }
}



void Firefox::reloadConfiguration()
{
    KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General") );
    if (QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        KConfigGroup grp = config;
        /* This allows the user to specify a profile database */
        m_dbFile = grp.readEntry<QString>("dbfile", QLatin1String(""));
        if (m_dbFile.isEmpty() || !QFile::exists(m_dbFile)) {
            //Try to get the right database file, the default profile is used
            KConfig firefoxProfile(QDir::homePath() + "/.mozilla/firefox/profiles.ini",
                                   KConfig::SimpleConfig);
            QStringList profilesList = firefoxProfile.groupList();
            profilesList = profilesList.filter(QRegExp(QStringLiteral("^Profile\\d+$")));
            int size = profilesList.size();

            QString profilePath;
            if (size == 1) {
                // There is only 1 profile so we select it
                KConfigGroup fGrp = firefoxProfile.group(profilesList.first());
                profilePath = fGrp.readEntry("Path", "");
            } else {
                // There are multiple profiles, find the default one
                foreach(const QString & profileName, profilesList) {
                    KConfigGroup fGrp = firefoxProfile.group(profileName);
                    if (fGrp.readEntry<int>("Default", 0)) {
                        profilePath = fGrp.readEntry("Path", "");
                        break;
                    }
                }
            }

            if (profilePath.isEmpty()) {
                //qDebug() << "No default firefox profile found";
                return;
            }
	    //qDebug() << "Profile " << profilePath << " found";
            profilePath.prepend(QStringLiteral("%1/.mozilla/firefox/").arg(QDir::homePath()));
            m_dbFile = profilePath + "/places.sqlite";
            grp.writeEntry("dbfile", m_dbFile);
            m_dbFile_fav = profilePath + "/favicons.sqlite";
        } else {
            auto dir = QDir(m_dbFile);
            if (dir.cdUp()) {
                QString profilePath = dir.absolutePath();
                m_dbFile_fav = profilePath + "/favicons.sqlite";
            }
        }
    } else {
        //qDebug() << "SQLITE driver isn't available";
    }
}
