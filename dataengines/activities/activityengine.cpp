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

#include "activityengine.h"
#include "activityservice.h"
#include "ActivityRankingInterface.h"

#include <kactivities/controller.h>
#include <kactivities/info.h>

#include <QApplication>
#include <QDBusServiceWatcher>

#define ACTIVITYMANAGER_SERVICE "org.kde.kactivitymanagerd"
#define ACTIVITYRANKING_OBJECT "/ActivityRanking"

ActivityEngine::ActivityEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args)
{
    Q_UNUSED(args);
    init();
}

void ActivityEngine::init()
{
    if (qApp->applicationName() == "plasma-netbook") {
        //hack for the netbook
        //FIXME can I read a setting or something instead?
    } else {
        m_activityController = new KActivities::Controller(this);
        m_currentActivity = m_activityController->currentActivity();
        QStringList activities = m_activityController->activities();
        //setData("allActivities", activities);
        foreach (const QString &id, activities) {
            insertActivity(id);
        }

        connect(m_activityController, SIGNAL(activityAdded(QString)), this, SLOT(activityAdded(QString)));
        connect(m_activityController, SIGNAL(activityRemoved(QString)), this, SLOT(activityRemoved(QString)));
        connect(m_activityController, SIGNAL(currentActivityChanged(QString)), this, SLOT(currentActivityChanged(QString)));

        //some convenience sources for times when checking every activity source would suck
        //it starts with _ so that it can easily be filtered out of sources()
        //maybe I should just make it not included in sources() instead?
        m_runningActivities = m_activityController->activities(KActivities::Info::Running);
        setData("Status", "Current", m_currentActivity);
        setData("Status", "Running", m_runningActivities);

        m_watcher = new QDBusServiceWatcher(
            ACTIVITYMANAGER_SERVICE,
            QDBusConnection::sessionBus(),
            QDBusServiceWatcher::WatchForRegistration
                | QDBusServiceWatcher::WatchForUnregistration,
            this);

        connect(m_watcher, SIGNAL(serviceRegistered(QString)),
                this, SLOT(enableRanking()));
        connect(m_watcher, SIGNAL(serviceUnregistered(QString)),
                this, SLOT(disableRanking()));

        if (QDBusConnection::sessionBus().interface()->isServiceRegistered(ACTIVITYMANAGER_SERVICE)) {
            enableRanking();
        }
    }
}

void ActivityEngine::insertActivity(const QString &id)
{
    //id -> name, icon, state
    KActivities::Info *activity = new KActivities::Info(id, this);
    m_activities[id] = activity;
    setData(id, "Name", activity->name());
    setData(id, "Icon", activity->icon());
    setData(id, "Current", m_currentActivity == id);

    QString state;
    switch (activity->state()) {
        case KActivities::Info::Running:
            state = "Running";
            break;
        case KActivities::Info::Starting:
            state = "Starting";
            break;
        case KActivities::Info::Stopping:
            state = "Stopping";
            break;
        case KActivities::Info::Stopped:
            state = "Stopped";
            break;
        case KActivities::Info::Invalid:
        default:
            state = "Invalid";
    }
    setData(id, "State", state);
    setData(id, "Score", m_activityScores.value(id));

    connect(activity, SIGNAL(infoChanged()), this, SLOT(activityDataChanged()));
    connect(activity, SIGNAL(stateChanged(KActivities::Info::State)), this, SLOT(activityStateChanged()));

    m_runningActivities << id;
}

void ActivityEngine::disableRanking()
{
    delete m_activityRankingClient;
}

void ActivityEngine::enableRanking()
{
    m_activityRankingClient = new org::kde::ActivityManager::ActivityRanking(
            ACTIVITYMANAGER_SERVICE,
            ACTIVITYRANKING_OBJECT,
            QDBusConnection::sessionBus()
        );
    connect(m_activityRankingClient, SIGNAL(rankingChanged(QStringList,ActivityDataList)),
            this, SLOT(rankingChanged(QStringList,ActivityDataList)));

    QDBusMessage msg = QDBusMessage::createMethodCall(ACTIVITYMANAGER_SERVICE,
                                                      ACTIVITYRANKING_OBJECT,
                                                      "org.kde.ActivityManager.ActivityRanking",
                                                      "activities");
    QDBusPendingReply<ActivityDataList> reply = QDBusConnection::sessionBus().asyncCall(msg);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(activityScoresReply(QDBusPendingCallWatcher*)));
}

void ActivityEngine::activityScoresReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ActivityDataList> reply = *watcher;
    if (reply.isError()) {
        qDebug() << "Error getting activity scores: " << reply.error().message();
    } else {
        setActivityScores(reply.value());
    }

    watcher->deleteLater();
}

void ActivityEngine::rankingChanged(const QStringList &topActivities, const ActivityDataList &activities)
{
    Q_UNUSED(topActivities)

    setActivityScores(activities);
}

void ActivityEngine::setActivityScores(const ActivityDataList &activities)
{
    QSet<QString> presentActivities;
    m_activityScores.clear();

    foreach (const ActivityData &activity, activities) {
        if (m_activities.contains(activity.id)) {
            setData(activity.id, "Score", activity.score);
        }
        presentActivities.insert(activity.id);
        m_activityScores[activity.id] = activity.score;
    }

    foreach (const QString &activityId, m_activityController->activities()) {
        if (!presentActivities.contains(activityId) && m_activities.contains(activityId)) {
            setData(activityId, "Score", 0);
        }
    }
}

void ActivityEngine::activityAdded(const QString &id)
{
    insertActivity(id);
    setData("Status", "Running", m_runningActivities);
}

void ActivityEngine::activityRemoved(const QString &id)
{
    removeSource(id);
    KActivities::Info *activity = m_activities.take(id);
    if (activity) {
        delete activity;
    }
    m_runningActivities.removeAll(id);
    setData("Status", "Running", m_runningActivities);
}

void ActivityEngine::currentActivityChanged(const QString &id)
{
    setData(m_currentActivity, "Current", false);
    m_currentActivity = id;
    setData(id, "Current", true);
    setData("Status", "Current", id);
}

void ActivityEngine::activityDataChanged()
{
    KActivities::Info *activity = qobject_cast<KActivities::Info*>(sender());
    if (!activity) {
        return;
    }
    setData(activity->id(), "Name", activity->name());
    setData(activity->id(), "Icon", activity->icon());
    setData(activity->id(), "Current", m_currentActivity == activity->id());
    setData(activity->id(), "Score", m_activityScores.value(activity->id()));
}

void ActivityEngine::activityStateChanged()
{
    KActivities::Info *activity = qobject_cast<KActivities::Info*>(sender());
    const QString id = activity->id();
    if (!activity) {
        return;
    }
    QString state;
    switch (activity->state()) {
        case KActivities::Info::Running:
            state = "Running";
            break;
        case KActivities::Info::Starting:
            state = "Starting";
            break;
        case KActivities::Info::Stopping:
            state = "Stopping";
            break;
        case KActivities::Info::Stopped:
            state = "Stopped";
            break;
        case KActivities::Info::Invalid:
        default:
            state = "Invalid";
    }
    setData(id, "State", state);

    if (activity->state() == KActivities::Info::Running) {
        if (!m_runningActivities.contains(id)) {
            m_runningActivities << id;
        }
    } else {
        m_runningActivities.removeAll(id);
    }

    setData("Status", "Running", m_runningActivities);
}


Plasma::Service *ActivityEngine::serviceForSource(const QString &source)
{
    //FIXME validate the name
    ActivityService *service = new ActivityService(m_activityController, source);
    service->setParent(this);
    return service;
}

K_EXPORT_PLASMA_DATAENGINE_WITH_JSON(activities, ActivityEngine, "plasma-engine-activities.json")

#include "activityengine.moc"
