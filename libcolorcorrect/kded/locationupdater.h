/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <kdedmodule.h>

#include <KConfigWatcher>

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

private:
    void resetLocator();
    void sendLocation(double latitude, double longitude);
    void disableSelf();

    ColorCorrect::CompositorAdaptor *const m_adaptor;
    ColorCorrect::Geolocator *m_locator = nullptr;
    KConfigWatcher::Ptr m_configWatcher;
};
