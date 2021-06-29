/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Glenn Ergeerts <marco.gulino@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
