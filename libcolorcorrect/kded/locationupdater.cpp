/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "locationupdater.h"

#include <KPluginFactory>

#include "../compositorcoloradaptor.h"
#include "../geolocator.h"

K_PLUGIN_CLASS_WITH_JSON(LocationUpdater, "colorcorrectlocationupdater.json")

LocationUpdater::LocationUpdater(QObject *parent, const QList<QVariant> &)
    : KDEDModule(parent)
{
    m_adaptor = new ColorCorrect::CompositorAdaptor(this);
    connect(m_adaptor, &ColorCorrect::CompositorAdaptor::dataUpdated, this, &LocationUpdater::resetLocator);
    resetLocator();
}

void LocationUpdater::resetLocator()
{
    if (m_adaptor->running() && m_adaptor->mode() == 0) {
        if (!m_locator) {
            m_locator = new ColorCorrect::Geolocator(this);
            connect(m_locator, &ColorCorrect::Geolocator::locationChanged, this, &LocationUpdater::sendLocation);
        }
    } else {
        delete m_locator;
        m_locator = nullptr;
    }
}

void LocationUpdater::sendLocation(double latitude, double longitude)
{
    m_adaptor->sendAutoLocationUpdate(latitude, longitude);
}

#include "locationupdater.moc"
