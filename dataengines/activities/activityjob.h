/*
 * Copyright 2009 Chani Armitage <chani@kde.org>
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

#ifndef ACTIVITYJOB_H
#define ACTIVITYJOB_H

// plasma
#include <Plasma/ServiceJob>

namespace KActivities
{
    class Controller;
} // namespace KActivities

class ActivityJob : public Plasma::ServiceJob
{

    Q_OBJECT

    public:
        ActivityJob(KActivities::Controller *controller, const QString &id, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = 0);
        ~ActivityJob();

    protected:
        void start();

    private:
        KActivities::Controller *m_activityController;
        QString m_id;

};

#endif // TASKJOB_H
