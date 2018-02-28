/*
 *   Copyright (C) 2008 Alex Merry <alex.merry@kdemail.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#include "placesengine.h"

#include <QString>
#include <QIcon>
#include <QDebug>

#include "placesproxymodel.h"
#include "placeservice.h"

PlacesEngine::PlacesEngine(QObject *parent, const QVariantList &args)
    : Plasma::DataEngine(parent, args)
{
    m_placesModel = new KFilePlacesModel(this);
    m_proxyModel = new PlacesProxyModel(this, m_placesModel);
    setModel(QStringLiteral("places"), m_proxyModel);
}

PlacesEngine::~PlacesEngine()
{
}

Plasma::Service *PlacesEngine::serviceForSource(const QString &source)
{
    if (source == QLatin1String("places")) {
        return new PlaceService(this, m_placesModel);
    }

    return DataEngine::serviceForSource(source);
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(places, PlacesEngine, "plasma-dataengine-places.json")

#include "placesengine.moc"

