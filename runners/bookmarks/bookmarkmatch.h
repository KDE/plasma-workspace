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

#ifndef BOOKMARKMATCH_H
#define BOOKMARKMATCH_H

#include <KRunner/QueryMatch>
#include <QIcon>
#include <QList>
#include <QString>

class BookmarkMatch
{
public:
    BookmarkMatch(const QIcon &icon,
                  const QString &searchTerm,
                  const QString &bookmarkTitle,
                  const QString &bookmarkURL,
                  const QString &description = QString());
    void addTo(QList<BookmarkMatch> &listOfResults, bool addEvenOnNoMatch);
    Plasma::QueryMatch asQueryMatch(Plasma::AbstractRunner *runner);

private:
    bool matches(const QString &search, const QString &matchingField);

private:
    QIcon m_icon;
    QString m_searchTerm;
    QString m_bookmarkTitle;
    QString m_bookmarkURL;
    QString m_description;
};

#endif // BOOKMARKMATCH_H
