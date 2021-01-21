/********************************************************************
Copyright 2017 Roman Gilg <subdiff@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
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
