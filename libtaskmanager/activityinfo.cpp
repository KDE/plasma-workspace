/*
    SPDX-FileCopyrightText: 2016 Eike Hein <hein@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "activityinfo.h"

#include <PlasmaActivities/ActivitiesModel>
#include <PlasmaActivities/Consumer>

namespace TaskManager
{
class Q_DECL_HIDDEN ActivityInfo::Private
{
public:
    Private(ActivityInfo *q);
    ~Private();

    static int instanceCount;
    static KActivities::Consumer *activityConsumer;
    static KActivities::ActivitiesModel *activitiesModel;
};

int ActivityInfo::Private::instanceCount = 0;
KActivities::Consumer *ActivityInfo::Private::activityConsumer = nullptr;
KActivities::ActivitiesModel *ActivityInfo::Private::activitiesModel = nullptr;

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

ActivityInfo::ActivityInfo(QObject *parent)
    : QObject(parent)
    , d(new Private(this))
{
    if (!d->activityConsumer) {
        d->activityConsumer = new KActivities::Consumer();
    }

    connect(d->activityConsumer, &KActivities::Consumer::currentActivityChanged, this, &ActivityInfo::currentActivityChanged);

    if (!d->activitiesModel) {
        d->activitiesModel = new KActivities::ActivitiesModel();
    }

    connect(d->activitiesModel, &KActivities::ActivitiesModel::modelReset, this, &ActivityInfo::namesOfRunningActivitiesChanged);

    connect(d->activitiesModel,
            &KActivities::ActivitiesModel::dataChanged,
            this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
                Q_UNUSED(topLeft)
                Q_UNUSED(bottomRight)

                if (roles.isEmpty() || roles.contains(Qt::DisplayRole)) {
                    Q_EMIT namesOfRunningActivitiesChanged();
                }
            });
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
    return d->activityConsumer->activities().count();
}

QStringList ActivityInfo::runningActivities() const
{
    return d->activityConsumer->activities();
}

QString ActivityInfo::activityName(const QString &id)
{
    KActivities::Info info(id);
    return info.name();
}

QString ActivityInfo::activityIcon(const QString &id)
{
    KActivities::Info info(id);
    return info.icon();
}
}

#include "moc_activityinfo.cpp"
