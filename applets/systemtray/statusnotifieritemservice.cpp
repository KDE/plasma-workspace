/*
    SPDX-FileCopyrightText: 2008 Alain Boyer <alainboyer@gmail.com>
    SPDX-FileCopyrightText: 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "statusnotifieritemservice.h"

// own
#include "statusnotifieritemjob.h"

StatusNotifierItemService::StatusNotifierItemService(StatusNotifierItemSource *source)
    : Plasma::Service(source)
    , m_source(source)
{
    setName(QStringLiteral("statusnotifieritem"));
}

Plasma::ServiceJob *StatusNotifierItemService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new StatusNotifierItemJob(m_source, operation, parameters, this);
}
