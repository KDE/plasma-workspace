/*
    SPDX-FileCopyrightText: 2007 Glenn Ergeerts <glenn.ergeerts@telenet.be>
    SPDX-FileCopyrightText: 2012 Marco Gulino <marco.gulino@xpeppers.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QList>
#include <QStandardPaths>
#include <QString>

#include "favicon.h"

class Profile
{
public:
    Profile(const QString &path, const QString &name, std::unique_ptr<Favicon> &&favicon)
        : m_path(path)
        , m_name(name)
        , m_favicon(std::move(favicon))
    {
        // Remove "Bookmarks" from end of path
        m_faviconSource = path.chopped(9) + QStringLiteral("Favicons");
        m_faviconCache = QStringLiteral("%1/bookmarksrunner/KRunner-Chrome-Favicons-%2.sqlite")
                             .arg(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation), name);
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
        return m_favicon.get();
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
    std::unique_ptr<Favicon> m_favicon;
    QString m_faviconSource;
    QString m_faviconCache;
};

class FindProfile
{
public:
    virtual std::vector<std::unique_ptr<Profile>> find() = 0;
    virtual ~FindProfile() = default;
};
