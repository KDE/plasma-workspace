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


#include "bookmarkmatch.h"
#include <QVariant>
#include "favicon.h"

// TODO: test

BookmarkMatch::BookmarkMatch(Favicon *favicon, const QString& searchTerm, const QString& bookmarkTitle, const QString& bookmarkURL, const QString& description )
    : m_favicon(favicon), m_searchTerm(searchTerm), m_bookmarkTitle(bookmarkTitle), m_bookmarkURL(bookmarkURL), m_description(description)
{
}

Plasma::QueryMatch BookmarkMatch::asQueryMatch( Plasma::AbstractRunner* runner )
{
    Plasma::QueryMatch::Type type = Plasma::QueryMatch::NoMatch;
    qreal relevance = 0;

    if (m_bookmarkTitle.compare(m_searchTerm, Qt::CaseInsensitive) == 0 ||
          (!m_description.isEmpty() && m_description.compare(m_searchTerm, Qt::CaseInsensitive) == 0)
    ) {
        type = Plasma::QueryMatch::ExactMatch;
        relevance = 1.0;
    } else if (m_bookmarkTitle.contains(m_searchTerm, Qt::CaseInsensitive)) {
        type = Plasma::QueryMatch::PossibleMatch;
        relevance = 0.45;
    } else if (!m_description.isEmpty() && m_description.contains(m_searchTerm, Qt::CaseInsensitive)) {
        type = Plasma::QueryMatch::PossibleMatch;
        relevance = 0.3;
    } else if (m_bookmarkURL.contains(m_searchTerm, Qt::CaseInsensitive)) {
        type = Plasma::QueryMatch::PossibleMatch;
        relevance = 0.2;
    } else {
        type = Plasma::QueryMatch::PossibleMatch;
        relevance = 0.18;
    }
    
    bool isNameEmpty = m_bookmarkTitle.isEmpty();
    bool isDescriptionEmpty = m_description.isEmpty();

    Plasma::QueryMatch match(runner);
    match.setType(type);
    match.setRelevance(relevance);
    match.setIcon(m_favicon->iconFor(m_bookmarkURL));
    match.setSubtext(m_bookmarkURL);

    // Try to set the following as text in this order: name, description, url
    match.setText( isNameEmpty
                    ?
                    (!isDescriptionEmpty ? m_description : m_bookmarkURL)
                    :
                    m_bookmarkTitle );

    match.setData(m_bookmarkURL);
    return match;
}

void BookmarkMatch::addTo(QList< BookmarkMatch >& listOfResults, bool addEvenOnNoMatch)
{
  if(!addEvenOnNoMatch && ! (
    matches(m_searchTerm, m_bookmarkTitle) ||
    matches(m_searchTerm, m_description) ||
    matches(m_searchTerm, m_bookmarkURL)
  ))  {
    return;
  }
  listOfResults << *this;
}

bool BookmarkMatch::matches(const QString &search, const QString &matchingField)
{
  return !matchingField.simplified().isEmpty() && matchingField.contains(search, Qt::CaseInsensitive);
}


