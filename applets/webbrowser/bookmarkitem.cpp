/*  Copyright (C) 2008 Marco Martin <notmart@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "bookmarkitem.h"


#include <kbookmarkmanager.h>
#include <kicon.h>
#include <QDebug>

BookmarkItem::BookmarkItem(KBookmark &bookmark)
    : QStandardItem(),
      m_bookmark(bookmark)
{
}

BookmarkItem::~BookmarkItem()
{
}



KBookmark BookmarkItem::bookmark() const
{
    return m_bookmark;
}

void BookmarkItem::setBookmark(const KBookmark &bookmark)
{
    m_bookmark = bookmark;
}

QVariant BookmarkItem::data(int role) const
{
    if (m_bookmark.isNull()) {
        return QStandardItem::data(role);
    }

    switch (role)
    {
    case Qt::DisplayRole:
        return m_bookmark.text();
    case Qt::DecorationRole:
        if (m_bookmark.isGroup() && m_bookmark.icon().isNull()) {
            return KIcon("folder-bookmarks");
        } else {
            return KIcon(m_bookmark.icon());
        }
    case UrlRole:
        return m_bookmark.url().prettyUrl();
    default:
        return QStandardItem::data(role);
    }
}

