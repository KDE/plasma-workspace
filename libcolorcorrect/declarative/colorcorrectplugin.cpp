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

#include "colorcorrectplugin.h"
#include "compositorcoloradaptor.h"
#include "geolocator.h"
#include "suncalc.h"

#include <QQmlEngine>

namespace ColorCorrect
{

void ColorCorrectPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("org.kde.colorcorrect"));
    qmlRegisterType<CompositorAdaptor>(uri, 0, 1, "CompositorAdaptor");
    qmlRegisterType<Geolocator>(uri, 0, 1, "Geolocator");
    qmlRegisterType<SunCalc>(uri, 0, 1, "SunCalc");
}

}
