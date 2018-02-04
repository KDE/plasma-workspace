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

#include <KActivities/ActivitiesModel>
#include <KActivities/Consumer>

namespace TaskManager
{

class Q_DECL_HIDDEN ActivityInfo::Private
{
public:
    Private(ActivityInfo *q);
    ~Private();

    static int instanceCount;
    static KActivities::Consumer* activityConsumer;
    static KActivities::ActivitiesModel* activitiesModel;
};

int ActivityInfo::Private::instanceCount = 0;
KActivities::Consumer* ActivityInfo::Private::activityConsumer = nullptr;
KActivities::ActivitiesModel* ActivityInfo::Private::activitiesModel = nullptr;


ActivityInfo::Private::Private(ActivityInfo *)
{
    ++instanceCount;
}

ActivityInfo::Private::~Private()
{
    --instanceCount;

    if (!instanceCount) {
        delete activityConsumer;
        activityConsumer = nullptr;
        delete activitiesModel;
        activitiesModel = nullptr;
    }
}

ActivityInfo::ActivityInfo(QObject *parent) : QObject(parent)
    , d(new Private(this))
{
    if (!d->activityConsumer) {
        d->activityConsumer = new KActivities::Consumer();
    }

    connect(d->activityConsumer, &KActivities::Consumer::currentActivityChanged,
        this, &ActivityInfo::currentActivityChanged);
    connect(d->activityConsumer, &KActivities::Consumer::runningActivitiesChanged,
        this, &ActivityInfo::numberOfRunningActivitiesChanged);
    connect(d->activityConsumer, &KActivities::Consumer::runningActivitiesChanged,
        this, &ActivityInfo::namesOfRunningActivitiesChanged);

    if (!d->activitiesModel) {
        d->activitiesModel = new KActivities::ActivitiesModel();
        d->activitiesModel->setShownStates(QVector<KActivities::Info::State>{KActivities::Info::Running});
    }

    connect(d->activitiesModel, &KActivities::ActivitiesModel::modelReset,
        this, &ActivityInfo::namesOfRunningActivitiesChanged);

    connect(d->activitiesModel, &KActivities::ActivitiesModel::dataChanged, this,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
            Q_UNUSED(topLeft)
            Q_UNUSED(bottomRight)

            if (roles.isEmpty() || roles.contains(Qt::DisplayRole)) {
                emit namesOfRunningActivitiesChanged();
            }
        }
    );
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
