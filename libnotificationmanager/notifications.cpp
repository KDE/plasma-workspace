/*
    SPDX-FileCopyrightText: 2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "notifications.h"

#include <QConcatenateTablesProxyModel>
#include <QDebug>
#include <QMetaEnum>
#include <memory>

#include <KDescendantsProxyModel>
#include <KLocalizedString>
#include <KNotification>

#include "limitedrowcountproxymodel_p.h"
#include "notificationfilterproxymodel_p.h"
#include "notificationgroupcollapsingproxymodel_p.h"
#include "notificationgroupingproxymodel_p.h"
#include "notificationsmodel.h"
#include "notificationsortproxymodel_p.h"

#include "jobsmodel.h"

#include "settings.h"

#include "notification.h"

#include "utils_p.h"

#include "debug.h"

using namespace Qt::StringLiterals;
using namespace NotificationManager;

// This class is a workaround to https://bugreports.qt.io/browse/QTBUG-134210
// and https://bugs.kde.org/show_bug.cgi?id=500749
// if a model is added to an empty QConcatenateTablesProxyModel
// it will report zero columns and non-zero rows, causing a data inconsistence
// which causes an infinite recursion in NotificationFilterProxyModel::filterAcceptsRow
// We fix the number of columns to always be 1
// remove when the upstream bug is fixed
class SingleColumnConcatenateTables : public QConcatenateTablesProxyModel
{
public:
    using QConcatenateTablesProxyModel::QConcatenateTablesProxyModel;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return 1;
    }
};

class Q_DECL_HIDDEN Notifications::Private
{
public:
    explicit Private(Notifications *q);
    ~Private();

    void initSourceModels();
    void initProxyModels();

    void updateCount();

    bool showNotifications = true;
    bool showJobs = false;

    Notifications::GroupMode groupMode = Notifications::GroupDisabled;
    int groupLimit = 0;
    bool expandUnread = false;

    int activeNotificationsCount = 0;
    int expiredNotificationsCount = 0;

    int unreadNotificationsCount = 0;

    int activeJobsCount = 0;
    int jobsPercentage = 0;

    static bool isGroup(const QModelIndex &idx);
    static uint notificationId(const QModelIndex &idx);
    QModelIndex mapFromModel(const QModelIndex &idx) const;

    // NOTE when you add or re-arrange models make sure to update mapFromModel()!
    NotificationsModel::Ptr notificationsModel;
    JobsModel::Ptr jobsModel;
    std::shared_ptr<Settings> settings() const;

    QConcatenateTablesProxyModel *notificationsAndJobsModel = nullptr;

    NotificationFilterProxyModel *filterModel = nullptr;
    NotificationSortProxyModel *sortModel = nullptr;
    NotificationGroupingProxyModel *groupingModel = nullptr;
    NotificationGroupCollapsingProxyModel *groupCollapsingModel = nullptr;
    KDescendantsProxyModel *flattenModel = nullptr;

    LimitedRowCountProxyModel *limiterModel = nullptr;

private:
    Notifications *const q;
};

Notifications::Private::Private(Notifications *q)
    : q(q)
{
}

Notifications::Private::~Private()
{
}

void Notifications::Private::initSourceModels()
{
    Q_ASSERT(notificationsAndJobsModel); // initProxyModels must be called before initSourceModels

    if (showNotifications && !notificationsModel) {
        notificationsModel = NotificationsModel::createNotificationsModel();
        notificationsAndJobsModel->addSourceModel(notificationsModel.get());
        connect(notificationsModel.get(), &NotificationsModel::lastReadChanged, q, [this] {
            updateCount();
            Q_EMIT q->lastReadChanged();
        });
    } else if (!showNotifications && notificationsModel) {
        notificationsAndJobsModel->removeSourceModel(notificationsModel.get());
        disconnect(notificationsModel.get(), nullptr, q, nullptr); // disconnect all
        notificationsModel = nullptr;
    }

    if (showJobs && !jobsModel) {
        jobsModel = JobsModel::createJobsModel();
        notificationsAndJobsModel->addSourceModel(jobsModel.get());
        jobsModel->init();
    } else if (!showJobs && jobsModel) {
        notificationsAndJobsModel->removeSourceModel(jobsModel.get());
        jobsModel = nullptr;
    }
}

void Notifications::Private::initProxyModels()
{
    /* The data flow is as follows:
     * NOTE when you add or re-arrange models make sure to update mapFromModel()!
     *
     * NotificationsModel      JobsModel
     *        \\                 /
     *         \\               /
     *     QConcatenateTablesProxyModel
     *               |||
     *               |||
     *     NotificationFilterProxyModel
     *     (filters by urgency, whitelist, etc)
     *                |
     *                |
     *      NotificationSortProxyModel
     *      (sorts by urgency, date, etc)
     *                |
     * --- BEGIN: Only when grouping is enabled ---
     *                |
     *    NotificationGroupingProxyModel
     *    (turns list into tree grouped by app)
     *               //\\
     *               //\\
     *   NotificationGroupCollapsingProxyModel
     *   (limits number of tree leaves for expand/collapse feature)
     *                /\
     *                /\
     *       KDescendantsProxyModel
     *       (flattens tree back into a list for consumption in ListView)
     *                |
     * --- END: Only when grouping is enabled ---
     *                |
     *    LimitedRowCountProxyModel
     *    (limits the total number of items in the model)
     *                |
     *                |
     *               \o/ <- Happy user seeing their notifications
     */

    if (!notificationsAndJobsModel) {
        notificationsAndJobsModel = new SingleColumnConcatenateTables(q);
    }

    if (!filterModel) {
        filterModel = new NotificationFilterProxyModel();
        connect(filterModel, &NotificationFilterProxyModel::urgenciesChanged, q, &Notifications::urgenciesChanged);
        connect(filterModel, &NotificationFilterProxyModel::showExpiredChanged, q, &Notifications::showExpiredChanged);
        connect(filterModel, &NotificationFilterProxyModel::showDismissedChanged, q, &Notifications::showDismissedChanged);
        connect(filterModel, &NotificationFilterProxyModel::showAddedDuringInhibitionChanged, q, &Notifications::showAddedDuringInhibitionChanged);
        connect(filterModel, &NotificationFilterProxyModel::blacklistedDesktopEntriesChanged, q, &Notifications::blacklistedDesktopEntriesChanged);
        connect(filterModel, &NotificationFilterProxyModel::blacklistedNotifyRcNamesChanged, q, &Notifications::blacklistedNotifyRcNamesChanged);

        filterModel->setSourceModel(notificationsAndJobsModel);

        connect(filterModel, &QAbstractItemModel::rowsInserted, q, [this] {
            updateCount();
        });
        connect(filterModel, &QAbstractItemModel::rowsRemoved, q, [this] {
            updateCount();
        });
        connect(filterModel, &QAbstractItemModel::dataChanged, q, [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles) {
            Q_UNUSED(topLeft);
            Q_UNUSED(bottomRight);
            if (roles.isEmpty() || roles.contains(Notifications::UpdatedRole) || roles.contains(Notifications::ExpiredRole)
                || roles.contains(Notifications::JobStateRole) || roles.contains(Notifications::PercentageRole) || roles.contains(Notifications::ReadRole)) {
                updateCount();
            }
        });
    }

    if (!sortModel) {
        sortModel = new NotificationSortProxyModel(q);
        connect(sortModel, &NotificationSortProxyModel::sortModeChanged, q, &Notifications::sortModeChanged);
        connect(sortModel, &NotificationSortProxyModel::sortOrderChanged, q, &Notifications::sortOrderChanged);
    }

    if (!limiterModel) {
        limiterModel = new LimitedRowCountProxyModel(q);
        connect(limiterModel, &LimitedRowCountProxyModel::limitChanged, q, &Notifications::limitChanged);
    }

    if (groupMode == GroupApplicationsFlat) {
        if (!groupingModel) {
            groupingModel = new NotificationGroupingProxyModel(q);
            groupingModel->setSourceModel(filterModel);
        }

        if (!groupCollapsingModel) {
            groupCollapsingModel = new NotificationGroupCollapsingProxyModel(q);
            groupCollapsingModel->setLimit(groupLimit);
            groupCollapsingModel->setExpandUnread(expandUnread);
            groupCollapsingModel->setLastRead(q->lastRead());
            groupCollapsingModel->setSourceModel(groupingModel);
        }

        sortModel->setSourceModel(groupCollapsingModel);

        flattenModel = new KDescendantsProxyModel(q);
        flattenModel->setSourceModel(sortModel);

        limiterModel->setSourceModel(flattenModel);
    } else {
        sortModel->setSourceModel(filterModel);
        limiterModel->setSourceModel(sortModel);
        delete flattenModel;
        flattenModel = nullptr;
        delete groupingModel;
        groupingModel = nullptr;
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

    // We want to get the numbers after main filtering (urgencies, whitelists, etc)
    // but before any limiting or group limiting, hence asking the filterModel for advice
    // at which point notifications and jobs also have already been merged
    for (int i = 0; i < filterModel->rowCount(); ++i) {
        const QModelIndex idx = filterModel->index(i, 0);

        if (idx.data(Notifications::ExpiredRole).toBool() || idx.data(Notifications::WasAddedDuringInhibitionRole).toBool()) {
            ++expired;
        } else {
            ++active;
        }

        const bool read = idx.data(Notifications::ReadRole).toBool();
        if (!active && !read) {
            QDateTime date = idx.data(Notifications::UpdatedRole).toDateTime();
            if (!date.isValid()) {
                date = idx.data(Notifications::CreatedRole).toDateTime();
            }

            if (notificationsModel && date > notificationsModel->lastRead()) {
                ++unread;
            }
        }

        if (idx.data(Notifications::TypeRole).toInt() == Notifications::JobType) {
            if (idx.data(Notifications::JobStateRole).toInt() != Notifications::JobStateStopped) {
                ++jobs;

                totalPercentage += idx.data(Notifications::PercentageRole).toInt();
            }
        }
    }

    if (activeNotificationsCount != active) {
        activeNotificationsCount = active;
        Q_EMIT q->activeNotificationsCountChanged();
    }
    if (expiredNotificationsCount != expired) {
        expiredNotificationsCount = expired;
        Q_EMIT q->expiredNotificationsCountChanged();
    }
    if (unreadNotificationsCount != unread) {
        unreadNotificationsCount = unread;
        Q_EMIT q->unreadNotificationsCountChanged();
    }
    if (activeJobsCount != jobs) {
        activeJobsCount = jobs;
        Q_EMIT q->activeJobsCountChanged();
    }

    const int percentage = (jobs > 0 ? totalPercentage / jobs : 0);
    if (jobsPercentage != percentage) {
        jobsPercentage = percentage;
        Q_EMIT q->jobsPercentageChanged();
    }

    // TODO don't Q_EMIT in dataChanged
    Q_EMIT q->countChanged();
}

bool Notifications::Private::isGroup(const QModelIndex &idx)
{
    return idx.data(Notifications::IsGroupRole).toBool();
}

uint Notifications::Private::notificationId(const QModelIndex &idx)
{
    return idx.data(Notifications::IdRole).toUInt();
}

QModelIndex Notifications::Private::mapFromModel(const QModelIndex &idx) const
{
    QModelIndex resolvedIdx = idx;

    QAbstractItemModel *models[] = {
        notificationsAndJobsModel,
        filterModel,
        sortModel,
        groupingModel,
        groupCollapsingModel,
        flattenModel,
        limiterModel,
    };

    // TODO can we do this with a generic loop like mapFromModel
    while (resolvedIdx.isValid() && resolvedIdx.model() != q) {
        const auto *idxModel = resolvedIdx.model();

        // HACK try to find the model that uses the index' model as source
        bool found = false;
        for (QAbstractItemModel *model : models) {
            if (!model) {
                continue;
            }

            if (auto *proxyModel = qobject_cast<QAbstractProxyModel *>(model)) {
                if (proxyModel->sourceModel() == idxModel) {
                    resolvedIdx = proxyModel->mapFromSource(resolvedIdx);
                    found = true;
                    break;
                }
            } else if (auto *concatenateModel = qobject_cast<QConcatenateTablesProxyModel *>(model)) {
                if (idxModel == notificationsModel.get() || idxModel == jobsModel.get()) {
                    resolvedIdx = concatenateModel->mapFromSource(resolvedIdx);
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            break;
        }
    }
    return resolvedIdx;
}

std::shared_ptr<Settings> Notifications::Private::settings() const
{
    static std::weak_ptr<Settings> s_instance;
    if (!s_instance.expired()) {
        std::shared_ptr<Settings> ptr(new Settings());
        s_instance = ptr;
        return ptr;
    }
    return s_instance.lock();
}

Notifications::Notifications(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private(this))
{
    // The proxy models are always the same, just with different
    // properties set whereas we want to avoid loading a source model
    // e.g. notifications or jobs when we're not actually using them
    d->initProxyModels();

    // init source models when used from C++
    QMetaObject::invokeMethod(
        this,
        [this] {
            d->initSourceModels();
        },
        Qt::QueuedConnection);
}

Notifications::~Notifications() = default;

void Notifications::classBegin()
{
}

void Notifications::componentComplete()
{
    // init source models when used from QML
    d->initSourceModels();
}

int Notifications::limit() const
{
    return d->limiterModel->limit();
}

void Notifications::setLimit(int limit)
{
    d->limiterModel->setLimit(limit);
}

int Notifications::groupLimit() const
{
    return d->groupLimit;
}

void Notifications::setGroupLimit(int limit)
{
    if (d->groupLimit == limit) {
        return;
    }

    d->groupLimit = limit;
    if (d->groupCollapsingModel) {
        d->groupCollapsingModel->setLimit(limit);
    }
    Q_EMIT groupLimitChanged();
}

bool Notifications::expandUnread() const
{
    return d->expandUnread;
}

void Notifications::setExpandUnread(bool expand)
{
    if (d->expandUnread == expand) {
        return;
    }

    d->expandUnread = expand;
    if (d->groupCollapsingModel) {
        d->groupCollapsingModel->setExpandUnread(expand);
    }
    Q_EMIT expandUnreadChanged();
}

QWindow *Notifications::window() const
{
    return d->notificationsModel ? d->notificationsModel->window() : nullptr;
}

void Notifications::setWindow(QWindow *window)
{
    if (d->notificationsModel) {
        d->notificationsModel->setWindow(window);
    } else {
        qCWarning(NOTIFICATIONMANAGER) << "Setting window before initialising the model" << this << window;
    }
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

bool Notifications::showAddedDuringInhibition() const
{
    return d->filterModel->showAddedDuringInhibition();
}

void Notifications::setShowAddedDuringInhibition(bool show)
{
    d->filterModel->setShowAddedDuringInhibition(show);
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

QStringList Notifications::whitelistedDesktopEntries() const
{
    return d->filterModel->whitelistedDesktopEntries();
}

void Notifications::setWhitelistedDesktopEntries(const QStringList &whitelist)
{
    d->filterModel->setWhiteListedDesktopEntries(whitelist);
}

QStringList Notifications::whitelistedNotifyRcNames() const
{
    return d->filterModel->whitelistedNotifyRcNames();
}

void Notifications::setWhitelistedNotifyRcNames(const QStringList &whitelist)
{
    d->filterModel->setWhitelistedNotifyRcNames(whitelist);
}

bool Notifications::showNotifications() const
{
    return d->showNotifications;
}

void Notifications::setShowNotifications(bool show)
{
    if (d->showNotifications == show) {
        return;
    }

    d->showNotifications = show;
    d->initSourceModels();
    Q_EMIT showNotificationsChanged();
}

bool Notifications::showJobs() const
{
    return d->showJobs;
}

void Notifications::setShowJobs(bool show)
{
    if (d->showJobs == show) {
        return;
    }

    d->showJobs = show;
    d->initSourceModels();
    Q_EMIT showJobsChanged();
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

Qt::SortOrder Notifications::sortOrder() const
{
    return d->sortModel->sortOrder();
}

void Notifications::setSortOrder(Qt::SortOrder sortOrder)
{
    d->sortModel->setSortOrder(sortOrder);
}

Notifications::GroupMode Notifications::groupMode() const
{
    return d->groupMode;
}

void Notifications::setGroupMode(GroupMode groupMode)
{
    if (d->groupMode != groupMode) {
        d->groupMode = groupMode;
        d->initProxyModels();
        Q_EMIT groupModeChanged();
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
    if (d->notificationsModel) {
        return d->notificationsModel->lastRead();
    }
    return QDateTime();
}

void Notifications::setLastRead(const QDateTime &lastRead)
{
    // TODO jobs could also be unread?
    if (d->notificationsModel) {
        d->notificationsModel->setLastRead(lastRead);
    }
    if (d->groupCollapsingModel) {
        d->groupCollapsingModel->setLastRead(lastRead);
    }
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

QPersistentModelIndex Notifications::makePersistentModelIndex(const QModelIndex &idx) const
{
    return QPersistentModelIndex(idx);
}

void Notifications::expire(const QModelIndex &idx)
{
    switch (static_cast<Notifications::Type>(idx.data(Notifications::TypeRole).toInt())) {
    case Notifications::NotificationType:
        d->notificationsModel->expire(Private::notificationId(idx));
        break;
    case Notifications::JobType:
        d->jobsModel->expire(Utils::mapToModel(idx, d->jobsModel.get()));
        break;
    default:
        Q_UNREACHABLE();
    }
}

void Notifications::close(const QModelIndex &idx)
{
    if (idx.data(Notifications::IsGroupRole).toBool()) {
        const QModelIndex groupIdx = Utils::mapToModel(idx, d->groupingModel);
        if (!groupIdx.isValid()) {
            qCWarning(NOTIFICATIONMANAGER) << "Failed to find group model index for this item";
            return;
        }

        Q_ASSERT(groupIdx.model() == d->groupingModel);

        const int childCount = d->groupingModel->rowCount(groupIdx);
        for (int i = childCount - 1; i >= 0; --i) {
            const QModelIndex childIdx = d->groupingModel->index(i, 0, groupIdx);
            close(childIdx);
        }
        return;
    }

    if (!idx.data(Notifications::ClosableRole).toBool()) {
        return;
    }

    switch (static_cast<Notifications::Type>(idx.data(Notifications::TypeRole).toInt())) {
    case Notifications::NotificationType:
        d->notificationsModel->close(Private::notificationId(idx));
        break;
    case Notifications::JobType:
        d->jobsModel->close(Utils::mapToModel(idx, d->jobsModel.get()));
        break;
    default:
        Q_UNREACHABLE();
    }
}

void Notifications::configure(const QModelIndex &idx)
{
    if (!d->notificationsModel) {
        return;
    }

    // For groups just configure the application, not the individual event
    if (Private::isGroup(idx)) {
        const QString desktopEntry = idx.data(Notifications::DesktopEntryRole).toString();
        const QString notifyRcName = idx.data(Notifications::NotifyRcNameRole).toString();

        d->notificationsModel->configure(desktopEntry, notifyRcName, QString() /*eventId*/);
        return;
    }

    d->notificationsModel->configure(Private::notificationId(idx));
}

void Notifications::invokeDefaultAction(const QModelIndex &idx, InvokeBehavior behavior)
{
    if (d->notificationsModel) {
        d->notificationsModel->invokeDefaultAction(Private::notificationId(idx), behavior);
    }
}

void Notifications::invokeAction(const QModelIndex &idx, const QString &actionId, InvokeBehavior behavior)
{
    if (d->notificationsModel) {
        d->notificationsModel->invokeAction(Private::notificationId(idx), actionId, behavior);
    }
}

void Notifications::reply(const QModelIndex &idx, const QString &text, InvokeBehavior behavior)
{
    if (d->notificationsModel) {
        d->notificationsModel->reply(Private::notificationId(idx), text, behavior);
    }
}

void Notifications::startTimeout(const QModelIndex &idx)
{
    startTimeout(Private::notificationId(idx));
}

void Notifications::startTimeout(uint notificationId)
{
    if (d->notificationsModel) {
        d->notificationsModel->startTimeout(notificationId);
    }
}

void Notifications::stopTimeout(const QModelIndex &idx)
{
    if (d->notificationsModel) {
        d->notificationsModel->stopTimeout(Private::notificationId(idx));
    }
}

void Notifications::suspendJob(const QModelIndex &idx)
{
    if (d->jobsModel) {
        d->jobsModel->suspend(Utils::mapToModel(idx, d->jobsModel.get()));
    }
}

void Notifications::resumeJob(const QModelIndex &idx)
{
    if (d->jobsModel) {
        d->jobsModel->resume(Utils::mapToModel(idx, d->jobsModel.get()));
    }
}

void Notifications::killJob(const QModelIndex &idx)
{
    if (d->jobsModel) {
        d->jobsModel->kill(Utils::mapToModel(idx, d->jobsModel.get()));
    }
}

void Notifications::clear(ClearFlags flags)
{
    if (d->notificationsModel) {
        d->notificationsModel->clear(flags);
    }
    if (d->jobsModel) {
        d->jobsModel->clear(flags);
    }
}

QModelIndex Notifications::groupIndex(const QModelIndex &idx) const
{
    if (idx.data(Notifications::IsGroupRole).toBool()) {
        return idx;
    }

    if (idx.data(Notifications::IsInGroupRole).toBool()) {
        QModelIndex groupingIdx = Utils::mapToModel(idx, d->groupingModel);
        return d->mapFromModel(groupingIdx.parent());
    }

    qCWarning(NOTIFICATIONMANAGER) << "Cannot get group index for item that isn't a group or inside one";
    return QModelIndex();
}

void Notifications::collapseAllGroups()
{
    if (d->groupCollapsingModel) {
        d->groupCollapsingModel->collapseAll();
    }
}

void Notifications::showInhibitionSummary()
{
    int inhibited = 0;
    for (int i = 0, count = d->notificationsAndJobsModel->rowCount(); i < count; ++i) {
        const QModelIndex idx = d->notificationsAndJobsModel->index(i, 0);
        if (!idx.data(Notifications::ReadRole).toBool() && idx.data(Notifications::WasAddedDuringInhibitionRole).toBool()) {
            ++inhibited;
        }
    }

    if (!inhibited) {
        return;
    }

    KNotification::event(u"inhibitionSummary"_s,
                         i18nc("@title", "Unread Notifications"),
                         i18nc("@info", "%1 notifications were received while Do Not Disturb was active.", QString::number(inhibited)),
                         u"preferences-desktop-notification-bell"_s,
                         KNotification::CloseOnTimeout,
                         u"libnotificationmanager"_s);
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
    return Utils::roleNames();
}

#include "moc_notifications.cpp"
