/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "bookmarkmatch.h"
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QString>

class Browser
{
public:
    virtual ~Browser()
    {
    }
    virtual QList<BookmarkMatch> match(const QString &term, bool addEveryThing) = 0;
    virtual void prepare()
    {
    }

    enum CacheResult {
        Error,
        Copied,
        Unchanged,
    };

public Q_SLOTS:
    virtual void teardown()
    {
    }

protected:
    /*
     * Updates the cached file if the source has been modified
     */
    CacheResult updateCacheFile(const QString &source, const QString &cache)
    {
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

    QJsonArray readChromeFormatBookmarks(const QString &path)
    {
        QJsonArray bookmarks;
        QFile bookmarksFile(path);
        if (!bookmarksFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return bookmarks;
        }
        const QJsonDocument jdoc = QJsonDocument::fromJson(bookmarksFile.readAll());
        if (jdoc.isNull()) {
            return bookmarks;
        }
        const QJsonObject resultMap = jdoc.object();
        if (!resultMap.contains(QLatin1String("roots"))) {
            return bookmarks;
        }
        const QJsonObject entries = resultMap.value(QLatin1String("roots")).toObject();
        for (const QJsonValue &folder : entries) {
            parseFolder(folder.toObject(), bookmarks);
        }
        return bookmarks;
    }

private:
    void parseFolder(const QJsonObject &obj, QJsonArray &bookmarks)
    {
        const QJsonArray children = obj.value(QStringLiteral("children")).toArray();
        for (const QJsonValue &child : children) {
            const QJsonObject entry = child.toObject();
            if (entry.value(QLatin1String("type")).toString() == QLatin1String("folder"))
                parseFolder(entry, bookmarks);
            else {
                bookmarks.append(entry);
            }
        }
    }
};
