/*
 *   Copyright (C) 2011 Viranch Mehta <viranch.mehta@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
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
