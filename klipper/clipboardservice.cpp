/*
    SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
