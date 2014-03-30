/*
 * Copyright 2013 Heena Mahour <heena393@gmail.com>
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

#ifndef TASKWINDOWJOB_H
#define TASKWINDOWJOB_H

// plasma
#include <Plasma/ServiceJob>

/**
 * Task Window Job
 */
class TaskWindowJob : public Plasma::ServiceJob
{

    Q_OBJECT

    public:
        TaskWindowJob(const QString &source, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = NULL);
        ~TaskWindowJob();

    protected:
        void start();

    private:
        QString m_id;

};

#endif 
