/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Glenn Ergeerts <marco.gulino@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
