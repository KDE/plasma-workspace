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
#include "bookmarkmatch.h"
#include "bookmarks_debug.h"
#include "favicon.h"
#include "faviconfromblob.h"
#include "fetchsqlite.h"
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

Firefox::Firefox(QObject *parent)
    : QObject(parent)
    , m_favicon(new FallbackFavicon(this))
    , m_fetchsqlite(nullptr)
    , m_fetchsqlite_fav(nullptr)
{
    m_dbCacheFile = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/bookmarkrunnerfirefoxdbfile.sqlite");
    m_dbCacheFile_fav = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/bookmarkrunnerfirefoxfavdbfile.sqlite");
    reloadConfiguration();
}

Firefox::~Firefox()
{
    // Delete the cached databases
    if (!m_dbFile.isEmpty()) {
        QFile db_CacheFile(m_dbCacheFile);
        if (db_CacheFile.exists()) {
            db_CacheFile.remove();
        }
    }
    if (!m_dbFile_fav.isEmpty()) {
        QFile db_CacheFileFav(m_dbCacheFile_fav);
        if (db_CacheFileFav.exists()) {
            db_CacheFileFav.remove();
        }
    }
}

void Firefox::prepare()
{
    if (updateCacheFile(m_dbFile, m_dbCacheFile) != Error) {
        m_fetchsqlite = new FetchSqlite(m_dbCacheFile);
        m_fetchsqlite->prepare();
    }
    updateCacheFile(m_dbFile_fav, m_dbCacheFile_fav);
    m_favicon->prepare();
}

QList<BookmarkMatch> Firefox::match(const QString &term, bool addEverything)
{
    QList<BookmarkMatch> matches;
    if (!m_fetchsqlite) {
        return matches;
    }

    QString query;
    if (addEverything) {
        query = QStringLiteral(
            "SELECT moz_bookmarks.fk, moz_bookmarks.title, moz_places.url "
            "FROM moz_bookmarks, moz_places WHERE "
            "moz_bookmarks.type = 1 AND moz_bookmarks.fk = moz_places.id");
    } else {
        query = QStringLiteral(
            "SELECT moz_bookmarks.fk, moz_bookmarks.title, moz_places.url "
            "FROM moz_bookmarks, moz_places WHERE "
            "moz_bookmarks.type = 1 AND moz_bookmarks.fk = moz_places.id AND "
            "(moz_bookmarks.title LIKE :term OR moz_places.url LIKE :term)");
    }
    const QMap<QString, QVariant> bindVariables{
        {QStringLiteral(":term"), QStringLiteral("%%%1%%").arg(term)},
    };
    const QList<QVariantMap> results = m_fetchsqlite->query(query, bindVariables);
    QMultiMap<QString, QString> uniqueResults;
    for (const QVariantMap &result : results) {
        const QString title = result.value(QStringLiteral("title")).toString();
        const QUrl url = result.value(QStringLiteral("url")).toUrl();
        if (url.isEmpty() || url.scheme() == QLatin1String("place")) {
            // Don't use bookmarks with empty url or Firefox's "place:" scheme,
            // e.g. used for "Most Visited" or "Recent Tags"
            // qDebug() << "element " << url << " was not added";
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
        const QString url = (*result).first;
        BookmarkMatch bookmarkMatch(m_favicon->iconFor(url), term, (*result).second, url);
        bookmarkMatch.addTo(matches, addEverything);
    }

    return matches;
}

void Firefox::teardown()
{
    if (m_fetchsqlite) {
        m_fetchsqlite->teardown();
        delete m_fetchsqlite;
        m_fetchsqlite = nullptr;
    }
    m_favicon->teardown();
}

void Firefox::reloadConfiguration()
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        qCWarning(RUNNER_BOOKMARKS) << "SQLITE driver isn't available";
        return;
    }
    KConfigGroup grp(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), QStringLiteral("General"));
    /* This allows the user to specify a profile database */
    m_dbFile = grp.readEntry("dbfile", QString());
    if (m_dbFile.isEmpty() || !QFile::exists(m_dbFile)) {
        // Try to get the right database file, the default profile is used
        KConfig firefoxProfile(QDir::homePath() + "/.mozilla/firefox/profiles.ini", KConfig::SimpleConfig);
        QStringList profilesList = firefoxProfile.groupList();
        profilesList = profilesList.filter(QRegularExpression(QStringLiteral("^Profile\\d+$")));

        QString profilePath;
        if (profilesList.size() == 1) {
            // There is only 1 profile so we select it
            KConfigGroup fGrp = firefoxProfile.group(profilesList.first());
            profilePath = fGrp.readEntry("Path");
        } else {
            const QStringList installConfig = firefoxProfile.groupList().filter(QRegularExpression("^Install.*"));
            // The profile with Default=1 is not always the default profile, see BUG: 418526
            // If there is only one Install* group it contains the default profile
            if (installConfig.size() == 1) {
                profilePath = firefoxProfile.group(installConfig.first()).readEntry("Default");
            } else {
                // There are multiple profiles, find the default one
                for (const QString &profileName : qAsConst(profilesList)) {
                    KConfigGroup fGrp = firefoxProfile.group(profileName);
                    if (fGrp.readEntry<int>("Default", 0)) {
                        profilePath = fGrp.readEntry("Path");
                        break;
                    }
                }
            }
        }

        if (profilePath.isEmpty()) {
            qCWarning(RUNNER_BOOKMARKS) << "No default firefox profile found";
            return;
        }
        profilePath.prepend(QStringLiteral("%1/.mozilla/firefox/").arg(QDir::homePath()));
        m_dbFile = profilePath + "/places.sqlite";
        m_dbFile_fav = profilePath + "/favicons.sqlite";
    } else {
        auto dir = QDir(m_dbFile);
        if (dir.cdUp()) {
            QString profilePath = dir.absolutePath();
            m_dbFile_fav = profilePath + "/favicons.sqlite";
        }
    }
    // We can reuse the favicon instance over the lifetime of the plugin consequently the
    // icons that are already written to disk can be reused in multiple match sessions
    updateCacheFile(m_dbFile_fav, m_dbCacheFile_fav);
    m_fetchsqlite_fav = new FetchSqlite(m_dbCacheFile_fav, this);
    delete m_favicon;
    m_favicon = FaviconFromBlob::firefox(m_fetchsqlite_fav, this);
}
