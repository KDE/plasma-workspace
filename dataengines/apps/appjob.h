/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

// plasma
#include <Plasma/ServiceJob>

// own
#include "appsource.h"

class AppJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    AppJob(AppSource *source, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = nullptr);
    ~AppJob() override;

protected:
    void start() override;

private:
    AppSource *m_source;
};
