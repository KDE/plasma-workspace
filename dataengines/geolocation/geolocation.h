/*
    SPDX-FileCopyrightText: 2009 Petri Damstén <damu@iki.fi>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QTimer>

#include <Plasma/DataEngine>

#include "geolocationprovider.h"

class GeolocationProvider;

class Geolocation : public Plasma::DataEngine
{
    Q_OBJECT

public:
    Geolocation(QObject *parent, const QVariantList &args);
    ~Geolocation() override;
    virtual void init();
    QStringList sources() const override;

protected:
    bool sourceRequestEvent(const QString &name) override;
    bool updateSourceEvent(const QString &name) override;
    bool updatePlugins(GeolocationProvider::UpdateTriggers triggers);

protected Q_SLOTS:
    void networkStatusChanged(bool isOnline);
    void pluginAvailabilityChanged(GeolocationProvider *provider);
    void pluginUpdated();
    void actuallySetData();

private:
    Data m_data;
    EntryAccuracy m_accuracy;
    QList<GeolocationProvider *> m_plugins;
    QTimer m_updateTimer;
    QTimer m_networkChangedTimer;
};
