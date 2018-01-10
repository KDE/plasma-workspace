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
#ifndef LOCATIONUPDATER_H
#define LOCATIONUPDATER_H

#include <kdedmodule.h>

namespace ColorCorrect
{
class Geolocator;
class CompositorAdaptor;
}

class LocationUpdater : public KDEDModule
{
    Q_OBJECT
public:
    LocationUpdater(QObject* parent, const QList<QVariant> &);

public Q_SLOTS:
    void sendLocation(double latitude, double longitude);

private:
    void resetLocator();

    ColorCorrect::CompositorAdaptor *m_adaptor = nullptr;
    ColorCorrect::Geolocator *m_locator = nullptr;
};

#endif
