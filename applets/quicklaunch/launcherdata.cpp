/***************************************************************************
 *   Copyright (C) 2008 by Lukas Appelhans <l.appelhans@gmx.de>            *
 *   Copyright (C) 2010 - 2011 by Ingomar Wesp <ingomar@wesp.name>         *
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
#include "launcherdata.h"

// Qt
#include <Qt>
#include <QtGlobal>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtXml/QDomDocument>

// KDE
#include <KBookmark>
#include <KBookmarkGroup>
#include <KDesktopFile>
#include <KMimeType>
#include <KUrl>

namespace Quicklaunch {

LauncherData::LauncherData(const KUrl& url)
:
    // KUrl::url() takes care of improperly escaped characters and
    // resolves pure paths into file:/// URLs
    m_url(url.url()),
    m_name(),
    m_description(),
    m_icon()
{
    if (m_url.isLocalFile() &&
        KDesktopFile::isDesktopFile(m_url.toLocalFile())) {

        KDesktopFile f(m_url.toLocalFile());

        m_name = f.readName();
        m_description = f.readGenericName();
        m_icon = f.readIcon();

    } else {
        m_icon = KMimeType::iconNameForUrl(m_url);
    }

    if (m_name.isNull()) {
        m_name = m_url.fileName();
    }

    if (m_icon.isNull()) {
        m_icon = "unknown";
    }
}

LauncherData::LauncherData()
:
    m_url(),
    m_name(),
    m_description(),
    m_icon()
{}

KUrl LauncherData::url() const
{
    return m_url;
}

QString LauncherData::name() const
{
    return m_name;
}

QString LauncherData::description() const
{
    return m_description;
}

QString LauncherData::icon() const
{
    return m_icon;
}

bool LauncherData::operator==(const LauncherData& other) const
{
    return
        m_url == other.m_url &&
        m_name == other.m_name &&
        m_description == other.m_description &&
        m_icon == other.m_icon;
}

bool LauncherData::operator!=(const LauncherData& other) const
{
    return !(*this == other);
}

void LauncherData::populateMimeData(QMimeData *mimeData)
{
    // Use the bookmarks API to do the heavy lifting
    KBookmark::List bookmarkList;

    KBookmark bookmark =
        KBookmark::standaloneBookmark(m_name, m_url, m_icon);

    bookmark.setDescription(m_description);

    bookmarkList.append(bookmark);
    bookmarkList.populateMimeData(mimeData);
}

bool LauncherData::canDecode(const QMimeData *mimeData)
{
    if (KBookmark::List::canDecode(mimeData)) {

        QDomDocument tempDoc;
        return hasUrls(
            KBookmark::List::fromMimeData(mimeData, tempDoc));
    }

    // TODO: Maybe allow text/plain as well if it looks like a URL
    return false;
}

QList<LauncherData> LauncherData::fromMimeData(const QMimeData *mimeData)
{
    QList<LauncherData> data;

    if (KBookmark::List::canDecode(mimeData)) {

        QDomDocument tempDoc;
        QList<KUrl> urlList =
            extractUrls(KBookmark::List::fromMimeData(mimeData, tempDoc));

        Q_FOREACH(const KUrl& url, urlList) {
            data.append(LauncherData(url));
        }
    }

    // TODO: Maybe allow text/plain as well if it looks like a URL

    return data;
}

bool LauncherData::hasUrls(const QList<KBookmark> &bookmarkList)
{
    Q_FOREACH(const KBookmark& bookmark, bookmarkList) {
        if (bookmark.isGroup() && hasUrls(bookmark.toGroup())) {
            return true;
        }
        else if (!bookmark.isSeparator() && !bookmark.isNull()) {
            return true;
        }
    }

    return false;
}

bool LauncherData::hasUrls(const KBookmarkGroup &bookmarkGroup)
{
    for (KBookmark bookmark = bookmarkGroup.first();;
         bookmark = bookmarkGroup.next(bookmark)) {

        if (bookmark.isNull()) {
            break;
        }
        else if (bookmark.isGroup() && hasUrls(bookmark.toGroup())) {
            return true;
        }
        else if (!bookmark.isSeparator()) {
            return true;
        }
    }

    return false;
}

QList<KUrl> LauncherData::extractUrls(const QList<KBookmark> &bookmarkList)
{
    QList<KUrl> urlList;

    Q_FOREACH(const KBookmark& bookmark, bookmarkList) {
        if (bookmark.isGroup()) {
            urlList.append(extractUrls(bookmark.toGroup()));
        } else if (!bookmark.isSeparator()) {
            urlList.append(bookmark.url());
        }
    }

    return urlList;
}

QList<KUrl> LauncherData::extractUrls(const KBookmarkGroup &bookmarkGroup)
{
    QList<KUrl> urlList;

    for (KBookmark bookmark = bookmarkGroup.first();;
         bookmark = bookmarkGroup.next(bookmark)) {

        if (bookmark.isNull()) {
            break;
        }
        else if (bookmark.isGroup()) {
            urlList.append(extractUrls(bookmark.toGroup()));
        }
        else if (!bookmark.isSeparator()) {
            urlList.append(bookmark.url());
        }
    }
    return urlList;
}
}
