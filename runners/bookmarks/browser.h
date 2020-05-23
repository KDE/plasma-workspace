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

#ifndef BROWSER_H
#define BROWSER_H

#include <QObject>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QString>
#include "bookmarkmatch.h"

class Browser
{
public:
    virtual ~Browser() {}
    virtual QList<BookmarkMatch> match(const QString& term, bool addEveryThing) = 0;
    virtual void prepare() {}

    enum CacheResult{
        Error,
        Copied,
        Unchanged
    };

public Q_SLOTS:
    virtual void teardown() {}

protected:
    /*
     * Updates the cached file if the source has been modified
    */
    CacheResult updateCacheFile(const QString &source, const QString &cache) {
        if (source.isEmpty() || cache.isEmpty()) {
            return Error;
        }
        QFileInfo cacheInfo(cache);
        if (!QFileInfo::exists(cache) || !cacheInfo.isFile()) {
            return QFile(source).copy(cache) ? Copied : Error;
        }

        QFileInfo sourceInfo(source);
        if (sourceInfo.lastModified() > cacheInfo.lastModified()) {
            QFile::remove(cache);
            return QFile(source).copy(cache) ? Copied : Error;
        }
        return Unchanged;
    }
};


#endif // BROWSER_H
