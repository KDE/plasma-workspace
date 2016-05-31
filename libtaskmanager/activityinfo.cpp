/********************************************************************
Copyright 2016  Eike Hein <hein.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "activityinfo.h"

#include <KActivities/Consumer>

namespace TaskManager
{

class ActivityInfo::Private
{
public:
    Private(ActivityInfo *q);
    ~Private();

    QHash<QString, KActivities::Info *> activityInfos;

    static int instanceCount;
    static KActivities::Consumer* activityConsumer;

    void refreshActivityInfos();

private:
    ActivityInfo *q;
};

int ActivityInfo::Private::instanceCount = 0;
KActivities::Consumer* ActivityInfo::Private::activityConsumer = nullptr;

ActivityInfo::Private::Private(ActivityInfo *q)
    : q(q)
{
    ++instanceCount;
}

ActivityInfo::Private::~Private()
{
    --instanceCount;

    if (!instanceCount) {
        delete activityConsumer;
    }
}

void ActivityInfo::Private::refreshActivityInfos()
{
    QMutableHashIterator<QString, KActivities::Info *> it(activityInfos);

    // Cull invalid activities.
    while (it.hasNext()) {
        it.next();

        if (!it.value()->isValid()) {
            delete it.value();
            it.remove();
        }
    }

    // Find new activities and start listening for changes in their state.
    foreach(const QString &activity, activityConsumer->activities()) {
        if (!activityInfos.contains(activity)) {
            KActivities::Info *info = new KActivities::Info(activity, q);

            // By connecting to ourselves, we will immediately clean up when an
            // activity's state transitions to Invalid.
            connect(info, SIGNAL(stateChanged(KActivities::Info::State)),
                q, SLOT(refreshActivityInfos()));

            activityInfos.insert(activity, info);
        }
    }

    // Activity list or activity state changes -> number of running
    // activities may have changed.
    q->numberOfRunningActivitiesChanged();
}

ActivityInfo::ActivityInfo(QObject *parent) : QObject(parent)
    , d(new Private(this))
{
    if (!d->activityConsumer) {
        d->activityConsumer = new KActivities::Consumer();
    }

    d->refreshActivityInfos();

    connect(d->activityConsumer, &KActivities::Consumer::currentActivityChanged,
        this, &ActivityInfo::currentActivityChanged);
    connect(d->activityConsumer, SIGNAL(activitiesChanged(QStringList)),
        this, SLOT(refreshActivityInfos()));
}

ActivityInfo::~ActivityInfo()
{
}

QString ActivityInfo::currentActivity() const
{
    return d->activityConsumer->currentActivity();
}

int ActivityInfo::numberOfRunningActivities() const
{
    return d->activityConsumer->activities(KActivities::Info::State::Running).count();
}

QStringList ActivityInfo::runningActivities() const
{
    return d->activityConsumer->activities(KActivities::Info::State::Running);
}

QString ActivityInfo::activityName(const QString &id)
{
    KActivities::Info info(id);

    if (info.state() != KActivities::Info::Invalid) {
        return info.name();
    }

    return QString();
}

}

#include "moc_activityinfo.cpp"
