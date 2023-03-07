/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "placesengine.h"

#include <QDebug>
#include <QIcon>
#include <QString>

#include "placeservice.h"
#include "placesproxymodel.h"

PlacesEngine::PlacesEngine(QObject *parent, const QVariantList &args)
    : Plasma5Support::DataEngine(parent, args)
{
    m_placesModel = new KFilePlacesModel(this);
    m_proxyModel = new PlacesProxyModel(this, m_placesModel);
    setModel(QStringLiteral("places"), m_proxyModel);
}

PlacesEngine::~PlacesEngine()
{
}

Plasma5Support::Service *PlacesEngine::serviceForSource(const QString &source)
{
    if (source == QLatin1String("places")) {
        return new PlaceService(this, m_placesModel);
    }

    return DataEngine::serviceForSource(source);
}

K_PLUGIN_CLASS_WITH_JSON(PlacesEngine, "plasma-dataengine-places.json")

#include "placesengine.moc"
