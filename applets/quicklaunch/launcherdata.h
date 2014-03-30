/***************************************************************************
  *   Copyright (C) 2010 - 2011 by Ingomar Wesp <ingomar@wesp.name>        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/
#ifndef QUICKLAUNCH_LAUNCHERDATA_H
#define QUICKLAUNCH_LAUNCHERDATA_H

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QMimeData>
#include <QtCore/QList>
#include <QtCore/QString>

// KDE
#include <KBookmark>
#include <KUrl>

class QMimeData;
class KBookmarkGroup;

namespace Quicklaunch {

/**
 * The LauncherData class encapsulates all the data necessary to display a
 * launcher in quicklaunch (url, name, description and an icon). It also
 * provides methods to read / write from / to QMimeData objects or
 * KBookmarks.
 */
class LauncherData {

public:
    /**
     * Creates an ItemData object from the given @a url and tries to determine
     * name, description and icon automatically.
     *
     * @param url the url for the ItemData object
     */
    LauncherData(const KUrl& url);

    /**
     * Creates a null ItemData object.
     */
    LauncherData();

    KUrl url() const;
    QString name() const;
    QString description() const;
    QString icon() const;

    bool operator == (const LauncherData& other) const;
    bool operator != (const LauncherData& other) const;

    void populateMimeData(QMimeData *mimeData);

    static bool canDecode(const QMimeData *mimeData);
    static QList<LauncherData> fromMimeData(const QMimeData *mimeData);

private:
    KUrl m_url;
    QString m_name;
    QString m_description;
    QString m_icon;

    /**
     * Determines whether the given @a bookmarkList contains URLs
     * (non-empty elements that are neither separators nor groups).
     *
     * @param bookmarkList a list of bookmarks
     *
     * @return whether the given @a bookmarkList contains URLs that
     *  can be used to create ItemData objects.
     */
    static bool hasUrls(const QList<KBookmark> &bookmarkList);

    /**
     * Determines whether the given @a bookmarkGroup contains URLs
     * (non-empty elements that are neither separators nor groups).
     *
     * @param a bookmark group
     *
     * @return whether the given @a bookmarkGroup contains URLs that
     *  can be used to create ItemData objects.
     */
    static bool hasUrls(const KBookmarkGroup &bookmarkGroup);

    static QList<KUrl> extractUrls(const QList<KBookmark> &bookmarkList);
    static QList<KUrl> extractUrls(const KBookmarkGroup &bookmarkGroup);
};
}

#endif /* QUICKLAUNCH_LAUNCHERDATA_H */
