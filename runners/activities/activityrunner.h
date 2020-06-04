/*
 *   Copyright (C) 2011 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef ACTIVITYRUNNER_H
#define ACTIVITYRUNNER_H

#include <krunner/abstractrunner.h>

#include <KActivities/Controller>

Q_DECLARE_METATYPE(KActivities::Consumer::ServiceStatus)

class ActivityRunner : public Plasma::AbstractRunner
{
    Q_OBJECT

    public:
        ActivityRunner(QObject *parent, const QVariantList &args);
        ~ActivityRunner() override;

        void match(Plasma::RunnerContext &context) override;
        void run(const Plasma::RunnerContext &context, const Plasma::QueryMatch &action) override;

    private Q_SLOTS:
        void serviceStatusChanged(KActivities::Consumer::ServiceStatus status);

    private:
        void addMatch(const KActivities::Info &activity, QList<Plasma::QueryMatch> &matches);

        KActivities::Controller *m_activities;
        KActivities::Consumer *m_consumer;
        const QString m_keywordi18n;
        const QString m_keyword;
};

#endif
