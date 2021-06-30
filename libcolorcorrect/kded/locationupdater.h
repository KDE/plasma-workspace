/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

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
    LocationUpdater(QObject *parent, const QList<QVariant> &);

public Q_SLOTS:
    void sendLocation(double latitude, double longitude);

private:
    void resetLocator();

    ColorCorrect::CompositorAdaptor *m_adaptor = nullptr;
    ColorCorrect::Geolocator *m_locator = nullptr;
};
