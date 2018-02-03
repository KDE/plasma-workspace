/*
 * Copyright 2008 Alain Boyer <alainboyer@gmail.com>
 * Copyright (C) 2009 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "statusnotifieritemservice.h"

// own
#include "statusnotifieritemjob.h"

StatusNotifierItemService::StatusNotifierItemService(StatusNotifierItemSource *source) :
    Plasma::Service(source),
    m_source(source)
{
    setName(QStringLiteral("statusnotifieritem"));
}

StatusNotifierItemService::~StatusNotifierItemService()
{
}

Plasma::ServiceJob* StatusNotifierItemService::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    return new StatusNotifierItemJob(m_source, operation, parameters, this);
}
