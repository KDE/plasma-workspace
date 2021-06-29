/*
    SPDX-FileCopyrightText: 2011 Viranch Mehta <viranch.mehta@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "soliddeviceservice.h"
#include "soliddeviceengine.h"
#include "soliddevicejob.h"

SolidDeviceService::SolidDeviceService(SolidDeviceEngine *parent, const QString &source)
    : Plasma::Service(parent)
    , m_engine(parent)
{
    setName(QStringLiteral("soliddevice"));
    setDestination(source);
}

Plasma::ServiceJob *SolidDeviceService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    if (operation == QLatin1String("updateFreespace")) {
        m_engine->updateStorageSpace(destination());
        return nullptr;
    }

    return new SolidDeviceJob(m_engine, destination(), operation, parameters);
}
