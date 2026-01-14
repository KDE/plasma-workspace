/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "favicon.h"
#include "fetchsqlite.h"
#include <QIcon>

class FaviconFromBlob : public Favicon
{
    Q_OBJECT
public:
    static std::unique_ptr<FaviconFromBlob> chrome(const QString &profileDirectory);
    static std::unique_ptr<FaviconFromBlob> firefox(std::unique_ptr<FetchSqlite> &&fetchSqlite);
    static std::unique_ptr<FaviconFromBlob> falkon(const QString &profileDirectory);
    ~FaviconFromBlob() override;
    QIcon iconFor(const QString &url) override;

    FaviconFromBlob(const QString &profileName,
                    const QString &query,
                    const QString &blobColumn,
                    std::unique_ptr<FetchSqlite> &&fetchSqlite,
                    QObject *parent = nullptr);
public Q_SLOTS:
    void prepare() override;
    void teardown() override;

private:
    QString m_profileCacheDirectory;
    QString m_query;
    QString const m_blobcolumn;
    std::unique_ptr<FetchSqlite> m_fetchsqlite;
    void cleanCacheDirectory();
};
