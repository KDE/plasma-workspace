/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "hotplugservice.h"
#include "hotplugengine.h"
#include "hotplugjob.h"

HotplugService::HotplugService(HotplugEngine *parent, const QString &source)
    : Plasma::Service(parent)
    , m_engine(parent)
{
    setName(QStringLiteral("hotplug"));
    setDestination(source);
}

Plasma::ServiceJob *HotplugService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new HotplugJob(m_engine, destination(), operation, parameters, this);
}
