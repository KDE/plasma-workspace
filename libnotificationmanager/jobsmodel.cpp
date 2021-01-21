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
#include "jobsmodel_p.h"

#include "utils_p.h"

#include <QDebug>

#include <KJob>

#include <Plasma/DataEngine>
#include <Plasma/DataEngineConsumer>
#include <Plasma/ServiceJob>

#include "job.h"
#include "job_p.h"

using namespace NotificationManager;

JobsModel::JobsModel()
    : QAbstractListModel(nullptr)
    , d(new JobsModelPrivate(this))
{
    connect(d, &JobsModelPrivate::jobViewAboutToBeAdded, this, [this](int row, Job *job) {
        Q_UNUSED(job);
        beginInsertRows(QModelIndex(), row, row);
    });
    connect(d, &JobsModelPrivate::jobViewAdded, this, [this](int row) {
        Q_UNUSED(row);
        endInsertRows();
    });

    connect(d, &JobsModelPrivate::jobViewAboutToBeRemoved, this, [this](int row) {
        beginRemoveRows(QModelIndex(), row, row);
    });
    connect(d, &JobsModelPrivate::jobViewRemoved, this, [this](int row) {
        Q_UNUSED(row);
        endRemoveRows();
    });

    connect(d, &JobsModelPrivate::jobViewChanged, this, [this](int row, Job *job, const QVector<int> &roles) {
        Q_UNUSED(job);
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, roles);
    });

    connect(d, &JobsModelPrivate::serviceOwnershipLost, this, &JobsModel::serviceOwnershipLost);
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

bool JobsModel::init()
{
    return d->init();
}

bool JobsModel::isValid() const
{
    return d->m_valid;
}

QVariant JobsModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return QVariant();
    }

    Job *job = d->m_jobViews.at(index.row());

    switch (role) {
    case Notifications::IdRole:
        return job->id();
    case Notifications::TypeRole:
        return Notifications::JobType;
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
    case Notifications::SummaryRole:
        return job->summary();
    case Notifications::BodyRole:
        return job->text();
    case Notifications::DesktopEntryRole:
        return job->desktopEntry();
    case Notifications::ApplicationNameRole:
        return job->applicationName();
    case Notifications::ApplicationIconNameRole:
        return job->applicationIconName();

    case Notifications::JobStateRole:
        return job->state();
    case Notifications::PercentageRole:
        return job->percentage();
    case Notifications::JobErrorRole:
        return job->error();
    case Notifications::SuspendableRole:
        return job->suspendable();
    case Notifications::KillableRole:
        return job->killable();
    case Notifications::JobDetailsRole:
        return QVariant::fromValue(job);

    // successfully finished jobs timeout like a regular notifiation
    // whereas running or error'd jobs are persistent
    case Notifications::TimeoutRole:
        return job->state() == Notifications::JobStateStopped && !job->error() ? -1 : 0;
    case Notifications::ClosableRole:
        return job->state() == Notifications::JobStateStopped;

    case Notifications::ConfigurableRole:
        return false;
    case Notifications::ExpiredRole:
        return job->expired();
    case Notifications::DismissedRole:
        return job->dismissed();

    // A job is usually either a long lasting operation you're aware about
    // or a quick job you don't care about.
    // When it's running, it's there, when it failed, it's persistent.
    // There's hardly a reason why it should show up as "unread".
    case Notifications::ReadRole:
        return true;
    }

    return QVariant();
}

bool JobsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        return false;
    }

    Job *job = d->m_jobViews.at(index.row());

    switch (role) {
    case Notifications::DismissedRole:
        if (value.toBool() != job->dismissed()) {
            job->setDismissed(value.toBool());
            return true;
        }
        break;
    }

    return false;
}

int JobsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->m_jobViews.count();
}

QHash<int, QByteArray> JobsModel::roleNames() const
{
    return Utils::roleNames();
}

void JobsModel::close(const QModelIndex &idx)
{
    if (checkIndex(idx, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        d->removeAt(idx.row());
    }
}

void JobsModel::expire(const QModelIndex &idx)
{
    if (checkIndex(idx, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        d->m_jobViews.at(idx.row())->setExpired(true);
    }
}

void JobsModel::suspend(const QModelIndex &idx)
{
    if (checkIndex(idx, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        d->m_jobViews.at(idx.row())->suspend();
    }
}

void JobsModel::resume(const QModelIndex &idx)
{
    if (checkIndex(idx, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        d->m_jobViews.at(idx.row())->resume();
    }
}

void JobsModel::kill(const QModelIndex &idx)
{
    if (checkIndex(idx, QAbstractItemModel::CheckIndexOption::IndexIsValid)) {
        d->m_jobViews.at(idx.row())->kill();
    }
}

void JobsModel::clear(Notifications::ClearFlags flags)
{
    if (d->m_jobViews.isEmpty()) {
        return;
    }

    for (int i = d->m_jobViews.count() - 1; i >= 0; --i) {
        Job *job = d->m_jobViews.at(i);

        bool clear = (flags.testFlag(Notifications::ClearExpired) && job->expired());

        // Compared to notifications, the number of jobs is typically small
        // so for simplicity we can just delete one item at a time
        if (clear) {
            d->removeAt(i);
        }
    }
}
