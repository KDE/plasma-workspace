/*
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <QObject>
#include <QTimer>

#include <Plasma/DataEngine>

#include "geolocation_export.h"

typedef QHash<QString, int> EntryAccuracy;

class GEOLOCATION_EXPORT GeolocationProvider : public QObject
{
    Q_OBJECT

public:
    enum UpdateTrigger {
        ForcedUpdate = 0,
        SourceEvent = 1,
        NetworkConnected = 2,
    };
    Q_DECLARE_FLAGS(UpdateTriggers, UpdateTrigger)

    explicit GeolocationProvider(QObject *parent = nullptr, const QVariantList &args = QVariantList());
    void init(Plasma::DataEngine::Data *data, EntryAccuracy *accuracies);

    UpdateTriggers updateTriggers() const;
    int accuracy() const;
    bool isAvailable() const;
    bool requestUpdate(UpdateTriggers trigger);
    bool populateSharedData();

Q_SIGNALS:
    void updated();
    void availabilityChanged(GeolocationProvider *provider);

protected:
    void setAccuracy(int accuracy);
    void setIsAvailable(bool available);
    void setUpdateTriggers(UpdateTriggers triggers);
    virtual void init();
    virtual void update();

protected Q_SLOTS:
    void setData(const Plasma::DataEngine::Data &data);
    void setData(const QString &key, const QVariant &value);

private:
    Plasma::DataEngine::Data *m_sharedData;
    EntryAccuracy *m_sharedAccuracies;
    Plasma::DataEngine::Data m_data;
    QTimer m_updateTimer;
    int m_accuracy;
    UpdateTriggers m_updateTriggers;
    bool m_available : 1;
    bool m_updating : 1;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GeolocationProvider::UpdateTriggers)
