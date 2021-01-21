/*
 *   Copyright (C) 2007 Tobias Koenig <tokoe@kde.org>
 *   Copyright (C) 2008 Marco Martin <notmart@gmail.com>
 *   Copyright (C) 2013 Andrea Scarpino <scarpino@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
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

#ifndef FAVICONS_DATAENGINE_H
#define FAVICONS_DATAENGINE_H

#include <Plasma/DataEngine>

class FaviconProvider;

/**
 * This class provides favicons for websites
 *
 * the queries are just the url of websites we want to fetch an icon
 */
class FaviconsEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    FaviconsEngine(QObject *parent, const QVariantList &args);
    ~FaviconsEngine() override;

protected:
    bool sourceRequestEvent(const QString &identifier) override;

protected Q_SLOTS:
    bool updateSourceEvent(const QString &identifier) override;

private Q_SLOTS:
    void finished(FaviconProvider *);
    void error(FaviconProvider *);
};

#endif
