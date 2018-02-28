/*
 *   Copyright 2010 Chani Armitage <chani@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 or
 *   (at your option) any later version.
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

#ifndef ACTIVITY_ENGINE_H
#define ACTIVITY_ENGINE_H

#include <QHash>

#include <Plasma/Service>
#include <Plasma/DataEngine>

#include "ActivityData.h"
#include "ActivityRankingInterface.h"


class QDBusServiceWatcher;

class ActivityService;

namespace KActivities
{
    class Controller;
    class Info;
}


class ActivityEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    ActivityEngine(QObject* parent, const QVariantList& args);
    Plasma::Service *serviceForSource(const QString &source) override;
    void init();

public Q_SLOTS:
    void activityAdded(const QString &id);
    void activityRemoved(const QString &id);
    void currentActivityChanged(const QString &id);

    void activityDataChanged();
    void activityStateChanged();

    void disableRanking();
    void enableRanking();
    void rankingChanged(const QStringList &topActivities, const ActivityDataList &activities);
    void activityScoresReply(QDBusPendingCallWatcher *watcher);

private:
    void insertActivity(const QString &id);
    void setActivityScores(const ActivityDataList &activities);

    KActivities::Controller *m_activityController;
    QHash<QString, KActivities::Info *> m_activities;
    QStringList m_runningActivities;
    QString m_currentActivity;

    org::kde::ActivityManager::ActivityRanking *m_activityRankingClient;
    QDBusServiceWatcher *m_watcher;
    QHash<QString, qreal> m_activityScores;

    friend class ActivityService;
};

#endif // SEARCHLAUNCH_ENGINE_H
