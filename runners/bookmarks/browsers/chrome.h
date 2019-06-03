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


#ifndef CHROME_H
#define CHROME_H

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
    explicit Chrome(FindProfile *findProfile, QObject* parent = nullptr);
    ~Chrome() override;
    QList<BookmarkMatch> match(const QString &term, bool addEveryThing) override;
public Q_SLOTS:
    void prepare() override;
    void teardown() override;
private:
    void parseFolder(const QJsonObject &entry, ProfileBookmarks *profile);
    virtual QList<BookmarkMatch> match(const QString &term, bool addEveryThing, ProfileBookmarks *profileBookmarks);
    QList<ProfileBookmarks*> m_profileBookmarks;
    KDirWatch* m_watcher = nullptr;
    bool m_dirty;

};

#endif // CHROME_H
