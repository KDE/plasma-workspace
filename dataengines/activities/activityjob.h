/*
    SPDX-FileCopyrightText: 2009 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

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
    ActivityJob(KActivities::Controller *controller,
                const QString &id,
                const QString &operation,
                QMap<QString, QVariant> &parameters,
                QObject *parent = nullptr);
    ~ActivityJob() override;

protected:
    void start() override;

private:
    KActivities::Controller *m_activityController;
    QString m_id;
};
