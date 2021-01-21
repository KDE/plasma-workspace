/*
 *   Copyright (C) 2007 Marco Martin <notmart@gmail.com>
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

#include "favicons.h"

#include <QImage>
#include <QPixmap>

#include "faviconprovider.h"

FaviconsEngine::FaviconsEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
}

FaviconsEngine::~FaviconsEngine()
{
}

bool FaviconsEngine::updateSourceEvent(const QString &identifier)
{
    FaviconProvider *provider = new FaviconProvider(this, identifier);

    connect(provider, &FaviconProvider::finished, this, &FaviconsEngine::finished);
    connect(provider, &FaviconProvider::error, this, &FaviconsEngine::error);

    if (!provider->image().isNull()) {
        setData(provider->identifier(), QStringLiteral("Icon"), provider->image());
    }

    return true;
}

bool FaviconsEngine::sourceRequestEvent(const QString &identifier)
{
    setData(identifier, QPixmap());
    return updateSourceEvent(identifier);
}

void FaviconsEngine::finished(FaviconProvider *provider)
{
    setData(provider->identifier(), QStringLiteral("Icon"), provider->image());
    provider->deleteLater();
}

void FaviconsEngine::error(FaviconProvider *provider)
{
    setData(provider->identifier(), QImage());
    provider->deleteLater();
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(favicons, FaviconsEngine, "plasma-dataengine-favicons.json")

#include "favicons.moc"
