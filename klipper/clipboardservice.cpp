/********************************************************************
This file is part of the KDE project.

Copyright (C) 2014 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "clipboardservice.h"
#include "clipboardjob.h"

ClipboardService::ClipboardService(Klipper *klipper, const QString &uuid)
    : Plasma::Service()
    , m_klipper(klipper)
    , m_uuid(uuid)
{
    setName(QStringLiteral("org.kde.plasma.clipboard"));
}

Plasma::ServiceJob *ClipboardService::createJob(const QString &operation, QVariantMap &parameters)
{
    return new ClipboardJob(m_klipper, m_uuid, operation, parameters, this);
}
