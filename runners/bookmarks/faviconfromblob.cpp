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

#include "faviconfromblob.h"

#include <QDebug>
#include <QPixmap>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QPainter>
#include <QDir>
#include "bookmarksrunner_defs.h"
#include "fetchsqlite.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>

#define dbFileName m_profileCacheDirectory + QDir::separator() + "Favicons.sqlite"


class StaticQuery : public BuildQuery {
public:
    StaticQuery(const QString &query) : m_query(query) {}
    QString query(QSqlDatabase *database) const override {
      Q_UNUSED(database);
      return m_query;
    }
private:
    const QString m_query;
};

class ChromeQuery : public BuildQuery {
public:
    ChromeQuery() {}
    QString query(QSqlDatabase *database) const override {
      //qDebug() << "tables: " << database->tables();
      if(database->tables().contains(QStringLiteral("favicon_bitmaps")))
	return QStringLiteral("SELECT * FROM favicons " \
            "inner join icon_mapping on icon_mapping.icon_id = favicons.id " \
            "inner join favicon_bitmaps on icon_mapping.icon_id = favicon_bitmaps.icon_id " \
            "WHERE page_url = :url ORDER BY height desc LIMIT 1;");
	return QStringLiteral("SELECT * FROM favicons inner join icon_mapping " \
	"on icon_mapping.icon_id = favicons.id " \
	"WHERE page_url = :url LIMIT 1;");
    }
};

FaviconFromBlob *FaviconFromBlob::chrome(const QString &profileDirectory, QObject *parent)
{
    QString profileName = QFileInfo(profileDirectory).fileName();
    QString faviconCache = QStringLiteral("%1/KRunner-Chrome-Favicons-%2.sqlite")
            .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), profileName);
    FetchSqlite *fetchSqlite = new FetchSqlite(profileDirectory + "/Favicons", faviconCache, parent);
    return new FaviconFromBlob(profileName, new ChromeQuery(), QStringLiteral("image_data"), fetchSqlite, parent);
}

FaviconFromBlob *FaviconFromBlob::firefox(FetchSqlite *fetchSqlite, QObject *parent)
{

    QString faviconQuery = QStringLiteral("SELECT moz_favicons.data FROM moz_favicons" \
                                   " inner join moz_places ON moz_places.favicon_id = moz_favicons.id" \
                                   " WHERE moz_places.url = :url LIMIT 1;");
    return new FaviconFromBlob(QStringLiteral("firefox-default"), new StaticQuery(faviconQuery), QStringLiteral("data"), fetchSqlite, parent);
}


FaviconFromBlob::FaviconFromBlob(const QString &profileName, BuildQuery *buildQuery, const QString &blobColumn, FetchSqlite *fetchSqlite, QObject *parent)
    : Favicon(parent), m_buildQuery(buildQuery), m_blobcolumn(blobColumn), m_fetchsqlite(fetchSqlite)
{
    m_profileCacheDirectory = QStringLiteral("%1/KRunner-Favicons-%2")
            .arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), profileName);
    //qDebug() << "got cache directory: " << m_profileCacheDirectory;
    cleanCacheDirectory();
    QDir().mkpath(m_profileCacheDirectory);
}

FaviconFromBlob::~FaviconFromBlob()
{
    cleanCacheDirectory();
    delete m_buildQuery;
}

void FaviconFromBlob::prepare()
{
    m_fetchsqlite->prepare();
}

void FaviconFromBlob::teardown()
{
    m_fetchsqlite->teardown();
}

void FaviconFromBlob::cleanCacheDirectory()
{
    foreach(const QFileInfo &file, QDir(m_profileCacheDirectory).entryInfoList(QDir::NoDotAndDotDot)) {
        //qDebug() << "Removing file " << file.absoluteFilePath() << ": " <<
        QFile(file.absoluteFilePath()).remove();
    }
    QDir().rmdir(m_profileCacheDirectory);
}

QIcon FaviconFromBlob::iconFor(const QString &url)
{
    //qDebug() << "got url: " << url;
    QString fileChecksum = QString::number(qChecksum(url.toLatin1(), url.toLatin1().size()));
    QFile iconFile( m_profileCacheDirectory + QDir::separator() + fileChecksum + "_favicon" );
    if(iconFile.size() == 0)
        iconFile.remove();
    if(!iconFile.exists()) {
        QMap<QString,QVariant> bindVariables;
        bindVariables.insert(QStringLiteral("url"), url);
        QList<QVariantMap> faviconFound = m_fetchsqlite->query(m_buildQuery, bindVariables);
        if(faviconFound.isEmpty()) return defaultIcon();

        QByteArray iconData = faviconFound.first().value(m_blobcolumn).toByteArray();
        //qDebug() << "Favicon found: " << iconData.size() << " bytes";
	if(iconData.size() <=0)
            return defaultIcon();

        iconFile.open(QFile::WriteOnly);
        iconFile.write(iconData);
        iconFile.close();
    }
    return QIcon(iconFile.fileName());
}

