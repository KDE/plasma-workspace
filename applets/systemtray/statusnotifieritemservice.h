/*
    SPDX-FileCopyrightText: 2008 Alain Boyer <alainboyer@gmail.com>
    SPDX-FileCopyrightText: 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

// own
#include "statusnotifieritemsource.h"

// plasma
#include <Plasma/Service>
#include <Plasma/ServiceJob>

/**
 * StatusNotifierItem Service
 */
class StatusNotifierItemService : public Plasma::Service
{
    Q_OBJECT

public:
    explicit StatusNotifierItemService(StatusNotifierItemSource *source);
    ~StatusNotifierItemService() override;

protected:
    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

private:
    StatusNotifierItemSource *m_source;
};
