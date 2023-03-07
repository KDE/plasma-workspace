/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

// plasma
#include <Plasma5Support/Service>
#include <Plasma5Support/ServiceJob>

// own
#include "appsource.h"

/**
 * App Service
 */
class AppService : public Plasma5Support::Service
{
    Q_OBJECT

public:
    explicit AppService(AppSource *source);
    ~AppService() override;

protected:
    Plasma5Support::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    AppSource *m_source;
};
