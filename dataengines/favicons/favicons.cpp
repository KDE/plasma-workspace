/*
    SPDX-FileCopyrightText: 2007 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2013 Andrea Scarpino <scarpino@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "favicons.h"

#include <QImage>
#include <QPixmap>

#include "faviconprovider.h"

FaviconsEngine::FaviconsEngine(QObject *parent, const QVariantList &args)
    : Plasma5Support::DataEngine(parent, args)
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

K_PLUGIN_CLASS_WITH_JSON(FaviconsEngine, "plasma-dataengine-favicons.json")

#include "favicons.moc"
