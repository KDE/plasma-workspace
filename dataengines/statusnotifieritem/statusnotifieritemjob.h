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

#ifndef STATUSNOTIFIERITEMJOB_H
#define STATUSNOTIFIERITEMJOB_H

// Qt
#include <QMenu>

// own
#include "statusnotifieritemsource.h"

// plasma
#include <Plasma/ServiceJob>

/**
 * Task Job
 */
class StatusNotifierItemJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    StatusNotifierItemJob(StatusNotifierItemSource *source, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = nullptr);
    ~StatusNotifierItemJob() override;

protected:
    void start() override;

private Q_SLOTS:
    void activateCallback(bool success);
    void contextMenuReady(QMenu *menu);

private:
    StatusNotifierItemSource *m_source;

};

#endif // STATUSNOTIFIERITEMJOB_H
