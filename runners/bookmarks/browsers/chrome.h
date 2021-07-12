/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "browser.h"
#include "findprofile.h"

#include <QList>

#include <KDirWatch>

class QJsonObject;

class ProfileBookmarks;
class Chrome : public QObject, public Browser
{
    Q_OBJECT
public:
    explicit Chrome(FindProfile *findProfile, QObject *parent = nullptr);
    ~Chrome() override;
    QList<BookmarkMatch> match(const QString &term, bool addEveryThing) override;
public Q_SLOTS:
    void prepare() override;
    void teardown() override;

private:
    void parseFolder(const QJsonObject &entry, ProfileBookmarks *profile);
    virtual QList<BookmarkMatch> match(const QString &term, bool addEveryThing, ProfileBookmarks *profileBookmarks);
    QList<ProfileBookmarks *> m_profileBookmarks;
    KDirWatch *m_watcher = nullptr;
    bool m_dirty;
};
