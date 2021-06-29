/*
    SPDX-FileCopyrightText: 2010 Chani Armitage <chani@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QHash>

#include <Plasma/DataEngine>
#include <Plasma/Service>

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
    ActivityEngine(QObject *parent, const QVariantList &args);
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
