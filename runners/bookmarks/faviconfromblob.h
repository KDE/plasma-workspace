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

#ifndef FAVICONFROMBLOB_H
#define FAVICONFROMBLOB_H

#include "favicon.h"
#include "fetchsqlite.h"
#include <QIcon>

class FaviconFromBlob : public Favicon
{
    Q_OBJECT
public:
    static FaviconFromBlob *chrome(const QString &profileDirectory, QObject *parent = nullptr);
    static FaviconFromBlob *firefox(FetchSqlite *fetchSqlite, QObject *parent = nullptr);
    static FaviconFromBlob *falkon(const QString &profileDirectory, QObject *parent = nullptr);
    ~FaviconFromBlob() override;
    QIcon iconFor(const QString &url) override;

public Q_SLOTS:
    void prepare() override;
    void teardown() override;

private:
    FaviconFromBlob(const QString &profileName, const QString &query, const QString &blobColumn, FetchSqlite *fetchSqlite, QObject *parent = nullptr);
    QString m_profileCacheDirectory;
    QString m_query;
    QString const m_blobcolumn;
    FetchSqlite *m_fetchsqlite;
    void cleanCacheDirectory();
};

#endif // FAVICONFROMBLOB_H
