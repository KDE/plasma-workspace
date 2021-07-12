/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "browser.h"
#include <QDir>
#include <QSqlDatabase>

class Favicon;
class FetchSqlite;
class Firefox : public QObject, public Browser
{
    Q_OBJECT
public:
    explicit Firefox(const QString &firefoxConfigDir, QObject *parent = nullptr);
    ~Firefox() override;
    QList<BookmarkMatch> match(const QString &term, bool addEverything) override;
public Q_SLOTS:
    void teardown() override;
    void prepare() override;

private:
    QString m_dbFile;
    QString m_dbFile_fav;
    const QString m_dbCacheFile;
    const QString m_dbCacheFile_fav;
    Favicon *m_favicon;
    FetchSqlite *m_fetchsqlite;
    FetchSqlite *m_fetchsqlite_fav;
};
