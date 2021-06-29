/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

#include "hotplugengine.h"

#include <Plasma/ServiceJob>

class HotplugJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    HotplugJob(HotplugEngine *engine, const QString &destination, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = nullptr)
        : ServiceJob(destination, operation, parameters, parent)
        , m_engine(engine)
        , m_dest(destination)
    {
    }

    void start() override;

private:
    HotplugEngine *m_engine;
    QString m_dest;
};
