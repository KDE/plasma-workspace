/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
