/*
    SPDX-FileCopyrightText: 2017 Roman Gilg <subdiff@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <QGeoPositionInfoSource>

#include <KConfigWatcher>
#include <KDEDModule>

namespace ColorCorrect
{
class CompositorAdaptor;
}

class LocationUpdater : public KDEDModule
{
    Q_OBJECT
public:
    LocationUpdater(QObject *parent, const QList<QVariant> &);

private:
    void resetLocator();
    void disableSelf();

    ColorCorrect::CompositorAdaptor *const m_adaptor;
    QGeoPositionInfoSource *m_positionSource = nullptr;
    KConfigWatcher::Ptr m_configWatcher;
};
