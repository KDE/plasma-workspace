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

#ifndef FAVICON_H
#define FAVICON_H

#include <QIcon>
#include <QObject>

class Favicon : public QObject
{
    Q_OBJECT
public:
    explicit Favicon(QObject *parent = nullptr);
    virtual QIcon iconFor(const QString &url) = 0;

protected:
    inline QIcon defaultIcon() const
    {
        return m_default_icon;
    }

private:
    QIcon const m_default_icon;

public Q_SLOTS:
    virtual void prepare()
    {
    }
    virtual void teardown()
    {
    }
};

class FallbackFavicon : public Favicon
{
    Q_OBJECT
public:
    FallbackFavicon(QObject *parent = nullptr)
        : Favicon(parent)
    {
    }
    QIcon iconFor(const QString &) override
    {
        return defaultIcon();
    }
};

#endif // FAVICON_H
