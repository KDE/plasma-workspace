/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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

    Q_REQUIRED_RESULT QString bookmarkTitle() const
    {
        return m_bookmarkTitle;
    }

    Q_REQUIRED_RESULT QString bookmarkUrl() const
    {
        return m_bookmarkURL;
    }

private:
    bool matches(const QString &search, const QString &matchingField);

private:
    QIcon m_icon;
    QString m_searchTerm;
    QString m_bookmarkTitle;
    QString m_bookmarkURL;
    QString m_description;
};
