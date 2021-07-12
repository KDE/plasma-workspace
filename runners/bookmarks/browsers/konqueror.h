/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "browser.h"
#include "favicon.h"
class KBookmarkManager;
class Favicon;

class KDEFavicon : public Favicon
{
    Q_OBJECT
public:
    explicit KDEFavicon(QObject *parent = nullptr);
    QIcon iconFor(const QString &url) override;
};

class Konqueror : public QObject, public Browser
{
    Q_OBJECT
public:
    explicit Konqueror(QObject *parent = nullptr);
    QList<BookmarkMatch> match(const QString &term, bool addEverything) override;

public Q_SLOTS:
    void teardown() override
    {
    }

private:
    KBookmarkManager *const m_bookmarkManager;
    Favicon *const m_favicon;
};
