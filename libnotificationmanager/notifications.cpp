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

#include "notifications.h"

#include <QSharedPointer>

#include <KConcatenateRowsProxyModel>
#include <KDescendantsProxyModel>

#include "notificationmodel.h"
#include "notificationfilterproxymodel_p.h"
#include "notificationsortproxymodel_p.h"
#include "notificationgroupingproxymodel_p.h"
#include "limitedrowcountproxymodel_p.h"

#include "jobsmodel.h"

#include "settings.h"

#include "notification.h"

#include "debug.h"

#include <QDebug>

using namespace NotificationManager;

class Q_DECL_HIDDEN Notifications::Private
{
public:
    explicit Private(Notifications *q);
    ~Private();

    void initModels();
    void updateCount();

    bool showJobs = false;

    Notifications::GroupMode groupMode = Notifications::GroupDisabled;

    int activeNotificationsCount = 0;
    int expiredNotificationsCount = 0;

    int unreadNotificationsCount = 0;

    int activeJobsCount = 0;
    int jobsPercentage = 0;

    static uint notificationId(const QModelIndex &idx);
    static QString jobId(const QModelIndex &idx);

    NotificationModel::Ptr notificationsModel;
    JobsModel::Ptr jobsModel;
    QSharedPointer<Settings> settings() const;

    KConcatenateRowsProxyModel *notificationsAndJobsModel = nullptr;

    NotificationFilterProxyModel *filterModel = nullptr;
    NotificationSortProxyModel *sortModel = nullptr;
    NotificationGroupingProxyModel *groupingModel = nullptr;
    KDescendantsProxyModel *flattenModel = nullptr;

    LimitedRowCountProxyModel *limiterModel = nullptr;

private:
    Notifications *q;
};

Notifications::Private::Private(Notifications *q)
    : q(q)
{

}

Notifications::Private::~Private()
{

}

void Notifications::Private::initModels()
{
    if (!notificationsModel) {
        notificationsModel = NotificationModel::createNotificationModel();
        connect(notificationsModel.data(), &NotificationModel::lastReadChanged, q, [this] {
            updateCount();
            emit q->lastReadChanged();
        });
    }

    if (!notificationsAndJobsModel) {
        notificationsAndJobsModel = new KConcatenateRowsProxyModel(q);
        notificationsAndJobsModel->addSourceModel(notificationsModel.data());
    }

    if (!filterModel) {
        filterModel = new NotificationFilterProxyModel();
        connect(filterModel, &NotificationFilterProxyModel::urgenciesChanged, q, &Notifications::urgenciesChanged);
        connect(filterModel, &NotificationFilterProxyModel::showExpiredChanged, q, &Notifications::showExpiredChanged);
        connect(filterModel, &NotificationFilterProxyModel::showDismissedChanged, q, &Notifications::showDismissedChanged);
        connect(filterModel, &NotificationFilterProxyModel::blacklistedDesktopEntriesChanged, q, &Notifications::blacklistedDesktopEntriesChanged);
        connect(filterModel, &NotificationFilterProxyModel::blacklistedNotifyRcNamesChanged, q, &Notifications::blacklistedNotifyRcNamesChanged);
        filterModel->setSourceModel(notificationsAndJobsModel);
    }

    if (!sortModel) {
        sortModel = new NotificationSortProxyModel(q);
        connect(sortModel, &NotificationSortProxyModel::sortModeChanged, q, &Notifications::sortModeChanged);
    }

    if (!limiterModel) {
        limiterModel = new LimitedRowCountProxyModel(q);
        connect(limiterModel, &LimitedRowCountProxyModel::limitChanged, q, &Notifications::limitChanged);
    }

    if (groupMode == GroupDisabled) {
        sortModel->setSourceModel(filterModel);
        limiterModel->setSourceModel(sortModel);
        delete flattenModel;
        flattenModel = nullptr;
        delete groupingModel;
        groupingModel = nullptr;
    } else if (groupMode == GroupApplicationsTree || groupMode == GroupApplicationsFlat) {
        if (!groupingModel) {
            groupingModel = new NotificationGroupingProxyModel(q);
            groupingModel->setSourceModel(filterModel);
        }

        sortModel->setSourceModel(groupingModel);

        if (groupMode == GroupApplicationsFlat) {
            flattenModel = new KDescendantsProxyModel(q);
            flattenModel->setSourceModel(sortModel);

            limiterModel->setSourceModel(flattenModel);
        } else {
            limiterModel->setSourceModel(groupingModel);

            delete flattenModel;
            flattenModel = nullptr;
        }
    }

    q->setSourceModel(limiterModel);
}

void Notifications::Private::updateCount()
{
    int active = 0;
    int expired = 0;
    int unread = 0;

    int jobs = 0;
    int totalPercentage = 0;

    for (int i = 0; i < q->rowCount(); ++i) {
        const QModelIndex idx = q->index(i, 0);

        // FIXME we don't show low urgency notifications in the popup
        // as such they never expire and keep the tray icon glowing indefinitely
        const bool isExpired = idx.data(Notifications::ExpiredRole).toBool();

        if (isExpired) {
            ++expired;
        }

        QDateTime date = idx.data(Notifications::UpdatedRole).toDateTime();
        if (!date.isValid()) {
            date = idx.data(Notifications::CreatedRole).toDateTime();
        }

        if (date > notificationsModel->lastRead()) {
            ++unread;
        }

        if (idx.data(Notifications::TypeRole).toInt() == Notifications::JobType) {
            if (idx.data(Notifications::JobStateRole).toInt() != Notifications::JobStateStopped) {
                ++jobs;

                totalPercentage += idx.data(Notifications::PercentageRole).toInt();
            }
        }

        if (!isExpired) {
            ++active;
        }
    }

    if (activeNotificationsCount != active) {
        activeNotificationsCount = active;
        emit q->activeNotificationsCountChanged();
    }
    if (expiredNotificationsCount != expired) {
        expiredNotificationsCount = expired;
        emit q->expiredNotificationsCountChanged();
    }
    if (unreadNotificationsCount != unread) {
        unreadNotificationsCount = unread;
        emit q->unreadNotificationsCountChanged();
    }
    if (activeJobsCount != jobs) {
        activeJobsCount = jobs;
        emit q->activeJobsCountChanged();
    }

    const int percentage = (jobs > 0 ? totalPercentage / jobs : 0);
    if (jobsPercentage != percentage) {
        jobsPercentage = percentage;
        emit q->jobsPercentageChanged();
    }

    // TODO don't emit in dataChanged
    emit q->countChanged();
}

uint Notifications::Private::notificationId(const QModelIndex &idx)
{
    return idx.data(Notifications::IdRole).toUInt();
}

QString Notifications::Private::jobId(const QModelIndex &idx)
{
    return idx.data(Notifications::IdRole).toString();
}

QSharedPointer<Settings> Notifications::Private::settings() const
{
    static QWeakPointer<Settings> s_instance;
    if (!s_instance) {
        QSharedPointer<Settings> ptr(new Settings());
        s_instance = ptr.toWeakRef();
        return ptr;
    }
    return s_instance.toStrongRef();
}

Notifications::Notifications(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private(this))
{
    d->initModels();

    connect(this, &QAbstractItemModel::rowsInserted, this, [this] {
        d->updateCount();
    });
    connect(this, &QAbstractItemModel::rowsRemoved, this, [this] {
        d->updateCount();
    });
    connect(this, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
        Q_UNUSED(topLeft);
        Q_UNUSED(bottomRight);
        if (roles.isEmpty()
                || roles.contains(Notifications::UpdatedRole)
                || roles.contains(Notifications::ExpiredRole)
                || roles.contains(Notifications::JobStateRole)
                || roles.contains(Notifications::PercentageRole)) {
            d->updateCount();
        }
    });
}

Notifications::~Notifications() = default;

// TODO why are we QQmlParserStatus if we don't use it?
void Notifications::classBegin()
{

}

void Notifications::componentComplete()
{

}

int Notifications::limit() const
{
    return d->limiterModel->limit();
}

void Notifications::setLimit(int limit)
{
    d->limiterModel->setLimit(limit);
}

bool Notifications::showExpired() const
{
    return d->filterModel->showExpired();
}

void Notifications::setShowExpired(bool show)
{
    d->filterModel->setShowExpired(show);
}

bool Notifications::showDismissed() const
{
    return d->filterModel->showDismissed();
}

void Notifications::setShowDismissed(bool show)
{
    d->filterModel->setShowDismissed(show);
}

QStringList Notifications::blacklistedDesktopEntries() const
{
    return d->filterModel->blacklistedDesktopEntries();
}

void Notifications::setBlacklistedDesktopEntries(const QStringList &blacklist)
{
    d->filterModel->setBlackListedDesktopEntries(blacklist);
}

QStringList Notifications::blacklistedNotifyRcNames() const
{
    return d->filterModel->blacklistedNotifyRcNames();
}

void Notifications::setBlacklistedNotifyRcNames(const QStringList &blacklist)
{
    d->filterModel->setBlacklistedNotifyRcNames(blacklist);
}

bool Notifications::showSuppressed() const
{
    // TODO
    return true;
}

void Notifications::setShowSuppressed(bool show)
{
    // TODO
    Q_UNUSED(show);
}

bool Notifications::showJobs() const
{
    return d->showJobs;
}

void Notifications::setShowJobs(bool showJobs)
{
    if (d->showJobs == showJobs) {
        return;
    }

    if (showJobs) {
        d->jobsModel = JobsModel::createJobsModel();
        d->notificationsAndJobsModel->addSourceModel(d->jobsModel.data());
    } else {
        d->notificationsAndJobsModel->removeSourceModel(d->jobsModel.data());
        d->jobsModel = nullptr;
    }
    d->showJobs = showJobs;
}

Notifications::Urgencies Notifications::urgencies() const
{
    return d->filterModel->urgencies();
}

void Notifications::setUrgencies(Urgencies urgencies)
{
    d->filterModel->setUrgencies(urgencies);
}

Notifications::SortMode Notifications::sortMode() const
{
    return d->sortModel->sortMode();
}

void Notifications::setSortMode(SortMode sortMode)
{
    d->sortModel->setSortMode(sortMode);
}

Notifications::GroupMode Notifications::groupMode() const
{
    return d->groupMode;
}

void Notifications::setGroupMode(GroupMode groupMode)
{
    if (d->groupMode != groupMode) {
        d->groupMode = groupMode;
        d->initModels();
        emit groupModeChanged();
    }
}

int Notifications::count() const
{
    return rowCount(QModelIndex());
}

int Notifications::activeNotificationsCount() const
{
    return d->activeNotificationsCount;
}

int Notifications::expiredNotificationsCount() const
{
    return d->expiredNotificationsCount;
}

QDateTime Notifications::lastRead() const
{
    return d->notificationsModel->lastRead();
}

void Notifications::setLastRead(const QDateTime &lastRead)
{
    d->notificationsModel->setLastRead(lastRead);
}

void Notifications::resetLastRead()
{
    setLastRead(QDateTime::currentDateTimeUtc());
}

int Notifications::unreadNotificationsCount() const
{
    return d->unreadNotificationsCount;
}

int Notifications::activeJobsCount() const
{
    return d->activeJobsCount;
}

int Notifications::jobsPercentage() const
{
    return d->jobsPercentage;
}

void Notifications::expire(const QModelIndex &idx)
{
    // TODO we could just call the NotificationServer directly?
    // FIXME just pass QModelIndex around

    switch (static_cast<Notifications::Type>(idx.data(Notifications::TypeRole).toInt())) {
    case Notifications::NotificationType:
        d->notificationsModel->expire(Private::notificationId(idx));
        break;
    case Notifications::JobType:
        d->jobsModel->expire(Private::jobId(idx));
        break;
    default:
        Q_UNREACHABLE();
    }
}

void Notifications::close(const QModelIndex &idx)
{
    switch (static_cast<Notifications::Type>(idx.data(Notifications::TypeRole).toInt())) {
    case Notifications::NotificationType:
        d->notificationsModel->close(Private::notificationId(idx));
        break;
    case Notifications::JobType:
        d->jobsModel->close(Private::jobId(idx));
        break;
    default:
        Q_UNREACHABLE();
    }
}

void Notifications::closeGroup(const QModelIndex &idx)
{
    qDebug() << "TODO CLOSE GROUP" << idx;
}

void Notifications::configure(const QModelIndex &idx)
{
    d->notificationsModel->configure(Private::notificationId(idx));
}

void Notifications::invokeDefaultAction(const QModelIndex &idx)
{
    d->notificationsModel->invokeDefaultAction(Private::notificationId(idx));
}

void Notifications::invokeAction(const QModelIndex &idx, const QString &actionId)
{
    d->notificationsModel->invokeAction(Private::notificationId(idx), actionId);
}

void Notifications::suspendJob(const QModelIndex &idx)
{
    d->jobsModel->suspend(Private::jobId(idx));
}

void Notifications::resumeJob(const QModelIndex &idx)
{
    d->jobsModel->resume(Private::jobId(idx));
}

void Notifications::killJob(const QModelIndex &idx)
{
    d->jobsModel->kill(Private::jobId(idx));
}

void Notifications::clear(ClearFlags flags)
{
    d->notificationsModel->clear(flags);
    d->jobsModel->clear(flags);
}

QVariant Notifications::data(const QModelIndex &index, int role) const
{
    return QSortFilterProxyModel::data(index, role);
}

bool Notifications::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return QSortFilterProxyModel::setData(index, value, role);
}

bool Notifications::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

bool Notifications::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}

int Notifications::rowCount(const QModelIndex &parent) const
{
    return QSortFilterProxyModel::rowCount(parent);
}

QHash<int, QByteArray> Notifications::roleNames() const
{
    return QHash<int, QByteArray> {
        {IdRole, QByteArrayLiteral("notificationId")}, // id is QML-reserved
        {IsGroupRole, QByteArrayLiteral("isGroup")},
        {IsInGroupRole, QByteArrayLiteral("isInGroup")},
        {TypeRole, QByteArrayLiteral("type")},
        {CreatedRole, QByteArrayLiteral("created")},
        {UpdatedRole, QByteArrayLiteral("updated")},
        {SummaryRole, QByteArrayLiteral("summary")},
        {BodyRole, QByteArrayLiteral("body")},
        {IconNameRole, QByteArrayLiteral("iconName")},
        {ImageRole, QByteArrayLiteral("image")},
        {DesktopEntryRole, QByteArrayLiteral("desktopEntry")},
        {NotifyRcNameRole, QByteArrayLiteral("notifyRcName")},

        {ApplicationNameRole, QByteArrayLiteral("applicationName")},
        {ApplicationIconNameRole, QByteArrayLiteral("applicationIconName")},
        {DeviceNameRole, QByteArrayLiteral("deviceName")},

        {ActionNamesRole, QByteArrayLiteral("actionNames")},
        {ActionLabelsRole, QByteArrayLiteral("actionLabels")},
        {HasDefaultActionRole, QByteArrayLiteral("hasDefaultAction")},

        {UrlsRole, QByteArrayLiteral("urls")},

        {UrgencyRole, QByteArrayLiteral("urgency")},
        {TimeoutRole, QByteArrayLiteral("timeout")},

        {ConfigurableRole, QByteArrayLiteral("configurable")},
        {ConfigureActionLabelRole, QByteArrayLiteral("configureActionLabel")},

        {JobStateRole, QByteArrayLiteral("jobState")},
        {PercentageRole, QByteArrayLiteral("percentage")},
        {ErrorRole, QByteArrayLiteral("error")},
        {ErrorTextRole, QByteArrayLiteral("errorText")},
        {SuspendableRole, QByteArrayLiteral("suspendable")},
        {KillableRole, QByteArrayLiteral("killable")},
        {JobDetailsRole, QByteArrayLiteral("jobDetails")},

        {ExpiredRole, QByteArrayLiteral("expired")},
        {DismissedRole, QByteArrayLiteral("dismissed")}
    };
}
