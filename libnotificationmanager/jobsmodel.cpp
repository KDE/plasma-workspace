/*
 * Copyright 2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "jobsmodel.h"

#include "notifications.h"

#include <QDebug>
#include <QScopedPointer>

#include <KJob>

#include <Plasma/DataEngineConsumer>
#include <Plasma/DataEngine>
#include <Plasma/ServiceJob>

#include "job.h"
#include "jobdetails.h"

using namespace NotificationManager;

class Q_DECL_HIDDEN JobsModel::Private
{
public:
    explicit Private(JobsModel *q);
    ~Private();

    void onSourceAdded(const QString &source);
    void onSourceRemoved(const QString &source);
    void operationCall(const QString &source, const QString &operation);

    JobsModel *q;

    QScopedPointer<Plasma::DataEngineConsumer> dataEngineConsumer;
    Plasma::DataEngine *dataEngine;

    QStringList sources;
    // FIXME why is this a pointer?
    // Try making it not anymore and make sure jobdetails has
    // a copy constructor that copies its private stuff
    QHash<QString, Job *> jobs;

};

JobsModel::Private::Private(JobsModel *q)
    : q(q)
    , dataEngineConsumer(new Plasma::DataEngineConsumer)
    , dataEngine(dataEngineConsumer->dataEngine(QStringLiteral("applicationjobs")))
{

    // NOTE cannot call onSourceAdded from constructor directly
    // because it does beginInsertRow and then blows up
    QMetaObject::invokeMethod(q, [this] {
        // FIXME why does connectAllSources() not work?
        const QStringList allSources = dataEngine->sources();
        for (const QString &source : allSources) {
            onSourceAdded(source);
        }
    }, Qt::QueuedConnection);

    QObject::connect(dataEngine, &Plasma::DataEngine::sourceAdded, q, [this](const QString &source) {
        onSourceAdded(source);
    });
    QObject::connect(dataEngine, &Plasma::DataEngine::sourceRemoved, q, [this](const QString &source) {
        onSourceRemoved(source);
    });
}

JobsModel::Private::~Private()
{
    qDeleteAll(jobs);
    jobs.clear();
}

void JobsModel::Private::onSourceAdded(const QString &source)
{
    q->beginInsertRows(QModelIndex(), sources.count(), sources.count());
    sources.append(source);
    jobs.insert(source, new Job(source));
    // must connect after adding so we process the initial update call
    dataEngine->connectSource(source, q);
    q->endInsertRows();
}

void JobsModel::Private::onSourceRemoved(const QString &source)
{
    const int row = sources.indexOf(source);
    // Job tracking might have been disabled in the meantime, otherwise this would not be neccessary
    if (row == -1) {
        return;
    }

    const QModelIndex idx = q->index(row, 0);
    Job *job = jobs.value(source);
    Q_ASSERT(job);

    dataEngine->disconnectSource(source, q);

    // When user canceled transfer, remove it from the model
    if (job->error() == 1) { // KIO::ERR_USER_CANCELED
        q->close(source);
        return;
    }

    // update timestamp
    job->setUpdated();

    // when it was hidden in history, bring it up again
    job->setDismissed(false);

    emit q->dataChanged(idx, idx, {
        Notifications::UpdatedRole,
        Notifications::DismissedRole,
        Notifications::TimeoutRole
    });
}

void JobsModel::Private::operationCall(const QString &source, const QString &operation)
{
    auto *service = dataEngine->serviceForSource(source);
    if (!service) {
        qWarning() << "Failed to get service for source" << source << "for operation" << operation;
        return;
    }

    auto *job = service->startOperationCall(service->operationDescription(operation));
    QObject::connect(job, &KJob::finished, service, &QObject::deleteLater);
}

JobsModel::JobsModel()
    : QAbstractListModel(nullptr)
    , d(new Private(this))
{
}

JobsModel::~JobsModel() = default;

JobsModel::Ptr JobsModel::createJobsModel()
{
    static QWeakPointer<JobsModel> s_instance;
    if (!s_instance) {
        QSharedPointer<JobsModel> ptr(new JobsModel());
        s_instance = ptr.toWeakRef();
        return ptr;
    }
    return s_instance.toStrongRef();
}

void JobsModel::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    const int row = d->sources.indexOf(sourceName);
    if (row == -1) {
        return;
    }

    const auto dirtyRoles = d->jobs.value(sourceName)->processData(data);

    if (!dirtyRoles.isEmpty()) {
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, dirtyRoles);
    }
}

QVariant JobsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= d->jobs.count()) {
        return QVariant();
    }

    const QString &sourceName = d->sources.at(index.row());
    const Job *job = d->jobs.value(sourceName);

    switch (role) {
    case Notifications::IdRole: return job->sourceName();
    case Notifications::TypeRole: return Notifications::JobType;
    // basically when it started
    case Notifications::CreatedRole:
        if (job->created().isValid()) {
            return job->created();
        }
        break;
    // basically when it finished
    case Notifications::UpdatedRole:
        if (job->updated().isValid()) {
            return job->updated();
        }
        break;
    case Notifications::SummaryRole: return job->summary();
    case Notifications::ApplicationNameRole: return job->applicationName();
    case Notifications::ApplicationIconNameRole: return job->applicationIconName();

    case Notifications::JobStateRole: return job->state();
    case Notifications::PercentageRole: return job->percentage();
    case Notifications::ErrorRole: return job->error();
    case Notifications::ErrorTextRole: return job->errorText();
    case Notifications::SuspendableRole: return job->suspendable();
    case Notifications::KillableRole: return job->killable();
    case Notifications::JobDetailsRole: return QVariant::fromValue(job->details());

    case Notifications::UrgencyRole: return Notifications::NormalUrgency;

    // successfully finished jobs timeout like a regular notifiation
    // whereas running or error'd jobs are persistent
    case Notifications::TimeoutRole:
        return job->state() == Notifications::JobStateStopped && !job->error() ? -1 : 0;

    case Notifications::ConfigurableRole: return false;
    case Notifications::ExpiredRole: return job->expired();
    case Notifications::DismissedRole: return job->dismissed();
    }

    return QVariant();
}

bool JobsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= d->jobs.count()) {
        return false;
    }

    const QString &sourceName = d->sources.at(index.row());
    Job *job = d->jobs.value(sourceName);

    bool dirty = false;

    switch (role) {
    case Notifications::DismissedRole:
        if (value.toBool() != job->dismissed()) {
            job->setDismissed(value.toBool());
            dirty = true;
        }
        break;
    }

    if (dirty) {
        emit dataChanged(index, index, {role});
    }

    return dirty;
}

int JobsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->sources.count();
}

void JobsModel::close(const QString &jobId)
{
    const int row = d->sources.indexOf(jobId);
    if (row == -1) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    d->sources.removeAt(row);
    delete d->jobs.take(jobId);
    endRemoveRows();
}

void JobsModel::expire(const QString &jobId)
{
    const int row = d->sources.indexOf(jobId);
    if (row == -1) {
        return;
    }

    const QModelIndex idx = index(row, 0);

    Job *job = d->jobs.value(jobId);
    job->setExpired(true);
    emit dataChanged(idx, idx, {Notifications::ExpiredRole});
}

void JobsModel::suspend(const QString &jobId)
{
    d->operationCall(jobId, QStringLiteral("suspend"));
}

void JobsModel::resume(const QString &jobId)
{
    d->operationCall(jobId, QStringLiteral("resume"));
}

void JobsModel::kill(const QString &jobId)
{
    d->operationCall(jobId, QStringLiteral("stop"));
}

void JobsModel::clear(Notifications::ClearFlags flags)
{
    Q_UNUSED(flags);
    // TODO
}
