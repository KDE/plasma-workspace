/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "appservice.h"

// own
#include "appjob.h"

AppService::AppService(AppSource *source)
    : Plasma::Service(source)
    , m_source(source)
{
    setName(QStringLiteral("apps"));
}

AppService::~AppService()
{
}

Plasma::ServiceJob *AppService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new AppJob(m_source, operation, parameters, this);
}
