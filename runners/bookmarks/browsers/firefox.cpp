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
    m_fetchsqlite(0)
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
        m_dbCacheFile = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("bookmarkrunnerfirefoxdbfile.sqlite");
    }
    if (!m_dbFile.isEmpty()) {
        m_fetchsqlite = new FetchSqlite(m_dbFile, m_dbCacheFile);
        m_fetchsqlite->prepare();

        delete m_favicon;
        m_favicon = 0;

        m_favicon = FaviconFromBlob::firefox(m_fetchsqlite, this);
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
        query = QStringLiteral("SELECT moz_bookmarks.fk, moz_bookmarks.title, moz_places.url," \
                    "moz_places.favicon_id FROM moz_bookmarks, moz_places WHERE " \
                    "moz_bookmarks.type = 1 AND moz_bookmarks.fk = moz_places.id");
    } else {
        const QString escapedTerm = tmpTerm.replace('\'', QLatin1String("\\'"));
        query = QString("SELECT moz_bookmarks.fk, moz_bookmarks.title, moz_places.url," \
                        "moz_places.favicon_id FROM moz_bookmarks, moz_places WHERE " \
                        "moz_bookmarks.type = 1 AND moz_bookmarks.fk = moz_places.id AND " \
                        "(moz_bookmarks.title LIKE  '%" + escapedTerm + "%' or moz_places.url LIKE '%"
                        + escapedTerm + "%')");
    }
    QList<QVariantMap> results = m_fetchsqlite->query(query, QMap<QString, QVariant>());
    foreach(QVariantMap result, results) {
        const QString title = result.value(QStringLiteral("title")).toString();
        const QUrl url = result.value(QStringLiteral("url")).toUrl();
        if (url.scheme().contains(QStringLiteral("place"))) {
            //Don't use bookmarks with empty title, url or Firefox intern url
            //qDebug() << "element " << url << " was not added";
            continue;
        }

        BookmarkMatch bookmarkMatch( m_favicon, term, title, url.toString());
        bookmarkMatch.addTo(matches, addEverything);
    }

    return matches;
}


void Firefox::teardown()
{
    if(m_fetchsqlite) {
        m_fetchsqlite->teardown();
        delete m_favicon;
        m_favicon = 0;
    }
}



void Firefox::reloadConfiguration()
{
    KConfigGroup config(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General") );
    if (QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        KConfigGroup grp = config;
        /* This allows the user to specify a profile database */
        m_dbFile = grp.readEntry<QString>("dbfile", QLatin1String(""));
        if (m_dbFile.isEmpty() || QFile::exists(m_dbFile)) {
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
        }
    } else {
        //qDebug() << "SQLITE driver isn't available";
    }
}
