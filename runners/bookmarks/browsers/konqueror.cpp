/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "konqueror.h"
#include "bookmarkmatch.h"

#include <QIcon>
#include <QStack>
#include <QUrl>

#include <KBookmarkManager>
#include <KIO/Global>

QIcon KDEFavicon::iconFor(const QString &url)
{
    const QString iconFile = KIO::favIconForUrl(QUrl(url));
    if (iconFile.isEmpty()) {
        return defaultIcon();
    }
    return QIcon::fromTheme(iconFile);
}

KDEFavicon::KDEFavicon(QObject *parent)
    : Favicon(parent)
{
}

Konqueror::Konqueror(QObject *parent)
    : QObject(parent)
    , m_bookmarkManager(KBookmarkManager::userBookmarksManager())
    , m_favicon(new KDEFavicon(this))
{
}

QList<BookmarkMatch> Konqueror::match(const QString &term, bool addEverything)
{
    KBookmarkGroup bookmarkGroup = m_bookmarkManager->root();

    QList<BookmarkMatch> matches;
    QStack<KBookmarkGroup> groups;

    KBookmark bookmark = bookmarkGroup.first();
    while (!bookmark.isNull()) {
        //         if (!context.isValid()) {
        //             return;
        //         } TODO: restore?

        if (bookmark.isSeparator()) {
            bookmark = bookmarkGroup.next(bookmark);
            continue;
        }

        if (bookmark.isGroup()) { // descend
            // qDebug(kdbg_code) << "descending into" << bookmark.text();
            groups.push(bookmarkGroup);
            bookmarkGroup = bookmark.toGroup();
            bookmark = bookmarkGroup.first();

            while (bookmark.isNull() && !groups.isEmpty()) {
                //                 if (!context.isValid()) {
                //                     return;
                //                 } TODO: restore?

                bookmark = bookmarkGroup;
                bookmarkGroup = groups.pop();
                bookmark = bookmarkGroup.next(bookmark);
            }

            continue;
        }

        const QString url = bookmark.url().url();
        BookmarkMatch bookmarkMatch(m_favicon->iconFor(url), term, bookmark.text(), url);
        bookmarkMatch.addTo(matches, addEverything);

        bookmark = bookmarkGroup.next(bookmark);
        while (bookmark.isNull() && !groups.isEmpty()) {
            //             if (!context.isValid()) {
            //                 return;
            //             } // TODO: restore?

            bookmark = bookmarkGroup;
            bookmarkGroup = groups.pop();
            ////qDebug() << "ascending from" << bookmark.text() << "to" << bookmarkGroup.text();
            bookmark = bookmarkGroup.next(bookmark);
        }
    }
    return matches;
}
