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
#ifndef FIND_PROFILE_H
#define FIND_PROFILE_H
#include <QList>
#include <QStandardPaths>
#include <QString>

class Favicon;
class Profile
{
public:
    Profile(const QString &path, const QString &name, Favicon *favicon)
        : m_path(path)
        , m_name(name)
        , m_favicon(favicon)
    {
        // Remove "Bookmarks" from end of path
        m_faviconSource = path.chopped(9) + QStringLiteral("Favicons");
        m_faviconCache = QStringLiteral("%1/KRunner-Chrome-Favicons-%2.sqlite").arg(QStandardPaths::writableLocation(QStandardPaths::CacheLocation), name);
    }
    inline QString path() const
    {
        return m_path;
    }
    inline QString name() const
    {
        return m_name;
    }
    inline Favicon *favicon() const
    {
        return m_favicon;
    }
    inline QString faviconSource() const
    {
        return m_faviconSource;
    }
    inline QString faviconCache() const
    {
        return m_faviconCache;
    }

private:
    QString m_path;
    QString m_name;
    Favicon *m_favicon;
    QString m_faviconSource;
    QString m_faviconCache;
};

class FindProfile
{
public:
    virtual QList<Profile> find() = 0;
    virtual ~FindProfile()
    {
    }
};

#endif
