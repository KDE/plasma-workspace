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

#include "taskgroupingproxymodel.h"
#include "abstracttasksmodel.h"
#include "tasktools.h"

#include <QSet>

namespace TaskManager
{

class Q_DECL_HIDDEN TaskGroupingProxyModel::Private
{
public:
    Private(TaskGroupingProxyModel *q);
    ~Private();

    AbstractTasksModelIface *abstractTasksSourceModel = nullptr;

    TasksModel::GroupMode groupMode = TasksModel::GroupApplications;
    bool groupDemandingAttention = false;
    int windowTasksThreshold = -1;

    QVector<QVector<int> *> rowMap;

    QSet<QString> blacklistedAppIds;
    QSet<QString> blacklistedLauncherUrls;

    bool isGroup(int row);
    bool any(const QModelIndex &parent, int role);
    bool all(const QModelIndex &parent, int role);

    void sourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceModelAboutToBeReset();
    void sourceModelReset();
    void sourceDataChanged(QModelIndex topLeft, QModelIndex bottomRight,
        const QVector<int> &roles = QVector<int>());
    void adjustMap(int anchor, int delta);

    void rebuildMap();
    bool shouldGroupTasks();
    void checkGrouping(bool silent = false);
    bool isBlacklisted(const QModelIndex &sourceIndex);
    bool tryToGroup(const QModelIndex &sourceIndex, bool silent = false);
    void formGroupFor(const QModelIndex &index);
    void breakGroupFor(const QModelIndex &index, bool silent = false);

private:
    TaskGroupingProxyModel *q;
};

TaskGroupingProxyModel::Private::Private(TaskGroupingProxyModel *q)
    : q(q)
{
}

TaskGroupingProxyModel::Private::~Private()
{
    qDeleteAll(rowMap);
}

bool TaskGroupingProxyModel::Private::isGroup(int row)
{
    if (row < 0 || row >= rowMap.count()) {
        return false;
    }

    return (rowMap.at(row)->count() > 1);
}

bool TaskGroupingProxyModel::Private::any(const QModelIndex &parent, int role)
{
    bool is = false;

    for (int i = 0; i < q->rowCount(parent); ++i) {
        if (q->index(i, 0, parent).data(role).toBool()) {
            return true;
        }
    }

    return is;
}

bool TaskGroupingProxyModel::Private::all(const QModelIndex &parent, int role)
{
    bool is = true;

    for (int i = 0; i < q->rowCount(parent); ++i) {
        if (!q->index(i, 0, parent).data(role).toBool()) {
            return false;
        }
    }

    return is;
}

void TaskGroupingProxyModel::Private::sourceRowsAboutToBeInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent)
    Q_UNUSED(first)
    Q_UNUSED(last)
}

void TaskGroupingProxyModel::Private::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    // We only support flat source models.
    if (parent.isValid()) {
        return;
    }

    adjustMap(start, (end - start) + 1);

    bool shouldGroup = shouldGroupTasks(); // Can be slightly expensive; cache return value.

    for (int i = start; i <= end; ++i) {
        if (!shouldGroup || !tryToGroup(q->sourceModel()->index(i, 0))) {
            q->beginInsertRows(QModelIndex(), rowMap.count(), rowMap.count());
            rowMap.append(new QVector<int>{i});
            q->endInsertRows();
        }
    }

    checkGrouping();
}

void TaskGroupingProxyModel::Private::sourceRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last)
{
    // We only support flat source models.
    if (parent.isValid()) {
        return;
    }

    for (int i = first; i <= last; ++i) {
        for (int j = 0; j < rowMap.count(); ++j) {
            const QVector<int> *sourceRows = rowMap.at(j);
            const int mapIndex = sourceRows->indexOf(i);

            if (mapIndex != -1) {
                // Remove top-level item.
                if (sourceRows->count() == 1) {
                    q->beginRemoveRows(QModelIndex(), j, j);
                    delete rowMap.takeAt(j);
                    q->endRemoveRows();
                // Dissolve group.
                } else if (sourceRows->count() == 2) {
                    const QModelIndex parent = q->index(j, 0);
                    q->beginRemoveRows(parent, 0, 1);
                    rowMap[j]->remove(mapIndex);
                    q->endRemoveRows();

                    // We're no longer a group parent.
                    q->dataChanged(parent, parent);
                // Remove group member.
                } else {
                    const QModelIndex parent = q->index(j, 0);
                    q->beginRemoveRows(parent, mapIndex, mapIndex);
                    rowMap[j]->remove(mapIndex);
                    q->endRemoveRows();

                    // Various roles of the parent evaluate child data, and the
                    // child list has changed.
                    q->dataChanged(parent, parent);
                }

                break;
            }
        }
    }
}

void TaskGroupingProxyModel::Private::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    // We only support flat source models.
    if (parent.isValid()) {
        return;
    }

    adjustMap(start + 1, -((end - start) + 1));

    checkGrouping();
}

void TaskGroupingProxyModel::Private::sourceModelAboutToBeReset()
{
    q->beginResetModel();
}

void TaskGroupingProxyModel::Private::sourceModelReset()
{
    rebuildMap();

    q->endResetModel();
}

void TaskGroupingProxyModel::Private::sourceDataChanged(QModelIndex topLeft, QModelIndex bottomRight, const QVector<int> &roles)
{
    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
        const QModelIndex &sourceIndex = q->sourceModel()->index(i, 0);
        QModelIndex proxyIndex = q->mapFromSource(sourceIndex);

        if (!proxyIndex.isValid()) {
            return;
        }

        const QModelIndex parent = proxyIndex.parent();

        // If a child item changes, its parent may need an update as well as many of
        // the data roles evaluate child data. See data().
        // TODO: Some roles do not need to bubble up as they fall through to the first
        // child in data(); it _might_ be worth adding constraints here later.
        if (parent.isValid()) {
            q->dataChanged(parent, parent, roles);
        }

        // When Private::groupDemandingAttention is false, tryToGroup() exempts tasks
        // which demand attention from being grouped. Therefore if this task is no longer
        // demanding attention, we need to try grouping it now.
        if (!parent.isValid()
            && !groupDemandingAttention && roles.contains(AbstractTasksModel::IsDemandingAttention)
            && !sourceIndex.data(AbstractTasksModel::IsDemandingAttention).toBool()) {

            if (shouldGroupTasks() && tryToGroup(sourceIndex)) {
                q->beginRemoveRows(QModelIndex(), proxyIndex.row(), proxyIndex.row());
                delete rowMap.takeAt(proxyIndex.row());
                q->endRemoveRows();
            } else {
                q->dataChanged(proxyIndex, proxyIndex, roles);
            }
        } else {
            q->dataChanged(proxyIndex, proxyIndex, roles);
        }
    }
}

void TaskGroupingProxyModel::Private::adjustMap(int anchor, int delta)
{
    for (int i = 0; i < rowMap.count(); ++i) {
        QVector<int> *sourceRows = rowMap.at(i);
        QMutableVectorIterator<int> it(*sourceRows);

        while (it.hasNext()) {
            it.next();

            if (it.value() >= anchor) {
                it.setValue(it.value() + delta);
            }
        }
    }
}

void TaskGroupingProxyModel::Private::rebuildMap()
{
    qDeleteAll(rowMap);
    rowMap.clear();

    const int rows = q->sourceModel()->rowCount();

    rowMap.reserve(rows);

    for (int i = 0; i < rows; ++i) {
        rowMap.append(new QVector<int>{i});
    }

    checkGrouping(true /* silent */);
}

bool TaskGroupingProxyModel::Private::shouldGroupTasks()
{
    if (groupMode == TasksModel::GroupDisabled) {
        return false;
    }

    if (windowTasksThreshold != -1) {
        // We're going to check the number of window tasks in the source model
        // against the grouping threshold. In practice that means we're ignoring
        // launcher and startup tasks. Startup tasks because they're very short-
        // lived (i.e. forming/breaking groups as they come and go would be very
        // noisy) and launcher tasks because we expect consumers to budget for
        // them in the threshold they set.
        int windowTasksCount = 0;

        for (int i = 0; i < q->sourceModel()->rowCount(); ++i) {
            const QModelIndex &idx = q->sourceModel()->index(i, 0);

            if (idx.data(AbstractTasksModel::IsWindow).toBool()) {
                ++windowTasksCount;
            }
        }

        return (windowTasksCount > windowTasksThreshold);
    }

    return true;
}

void TaskGroupingProxyModel::Private::checkGrouping(bool silent)
{
    if (shouldGroupTasks()) {
        for (int i = (rowMap.count()) - 1; i >= 0; --i) {
            if (isGroup(i)) {
                continue;
            }

            if (tryToGroup(q->sourceModel()->index(rowMap.at(i)->constFirst(), 0), silent)) {
                q->beginRemoveRows(QModelIndex(), i, i);
                delete rowMap.takeAt(i); // Safe since we're iterating backwards.
                q->endRemoveRows();
            }
        }
    } else {
        for (int i = (rowMap.count()) - 1; i >= 0; --i) {
            breakGroupFor(q->index(i, 0), silent);
        }
    }
}

bool TaskGroupingProxyModel::Private::isBlacklisted(const QModelIndex &sourceIndex)
{
    // Check app id against blacklist.
    if (blacklistedAppIds.count()
        && blacklistedAppIds.contains(sourceIndex.data(AbstractTasksModel::AppId).toString())) {
        return true;
    }

    // Check launcher URL (sans query items) against blacklist.
    if (blacklistedLauncherUrls.count()) {
        const QUrl &launcherUrl = sourceIndex.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl();
        const QString &launcherUrlString = launcherUrl.toString(QUrl::PrettyDecoded | QUrl::RemoveQuery);

        if (blacklistedLauncherUrls.contains(launcherUrlString)) {
            return true;
        }
    }

    return false;
}

bool TaskGroupingProxyModel::Private::tryToGroup(const QModelIndex &sourceIndex, bool silent)
{
    // NOTE: We only group window tasks at this time. If this ever changes, the
    // implementation of data() will have to be adjusted significantly, as for
    // many roles it currently falls through to the first child item when dealing
    // with requests for the parent (e.g. IsWindow).
    if (!sourceIndex.data(AbstractTasksModel::IsWindow).toBool()) {
        return false;
    }

    // If Private::groupDemandingAttention is false and this task is demanding
    // attention, don't group it at this time. We'll instead try to group it once
    // it no longer demands attention (see sourceDataChanged()).
    if (!groupDemandingAttention
        && sourceIndex.data(AbstractTasksModel::IsDemandingAttention).toBool()) {
        return false;
    }

    // Blacklist checks.
    if (isBlacklisted(sourceIndex)) {
        return false;
    }

    // Meat of the matter: Try to add this source row to a sub-list with source rows
    // associated with the same application.
    for (int i = 0; i < rowMap.count(); ++i) {
        const QModelIndex &groupRep = q->sourceModel()->index(rowMap.at(i)->constFirst(), 0);

        // Don't match a row with itself.
        if (sourceIndex == groupRep) {
            continue;
        }

        // Don't group windows with anything other than windows.
        if (!groupRep.data(AbstractTasksModel::IsWindow).toBool()) {
            continue;
        }

        if (appsMatch(sourceIndex, groupRep)) {
            const QModelIndex parent = q->index(i, 0);

            if (!silent) {
                const int newIndex = rowMap.at(i)->count();

                if (newIndex == 1) {
                    q->beginInsertRows(parent, 0, 1);
                } else {
                    q->beginInsertRows(parent, newIndex, newIndex);
                }
            }

            rowMap[i]->append(sourceIndex.row());

            if (!silent) {
                q->endInsertRows();

                q->dataChanged(parent, parent);
            }

            return true;
        }
    }

    return false;
}

void TaskGroupingProxyModel::Private::formGroupFor(const QModelIndex &index)
{
    // Already in group or a group.
    if (index.parent().isValid() || isGroup(index.row())) {
        return;
    }

    // We need to grab a source index as we may invalidate the index passed
    // in through grouping.
    const QModelIndex &sourceTarget = q->mapToSource(index);

    for (int i = (rowMap.count() - 1); i >= 0; --i) {
        const QModelIndex &sourceIndex = q->sourceModel()->index(rowMap.at(i)->constFirst(), 0);

        if (!appsMatch(sourceTarget, sourceIndex)) {
            continue;
        }

        if (tryToGroup(sourceIndex)) {
            q->beginRemoveRows(QModelIndex(), i, i);
            delete rowMap.takeAt(i); // Safe since we're iterating backwards.
            q->endRemoveRows();
        }
    }
}

void TaskGroupingProxyModel::Private::breakGroupFor(const QModelIndex &index, bool silent)
{
    const int row = index.row();

    if (!isGroup(row)) {
        return;
    }

    // The first child will move up to the top level.
    QVector<int> extraChildren = rowMap.at(row)->mid(1);

    // NOTE: We're going to do remove+insert transactions instead of a
    // single reparenting move transaction to save on complexity in the
    // proxies above us.
    // TODO: This could technically be optimized, though it's very
    // unlikely to be ever worth it.
    if (!silent) {
        q->beginRemoveRows(index, 0, extraChildren.count());
    }

    rowMap[row]->resize(1);

    if (!silent) {
        q->endRemoveRows();

        // We're no longer a group parent.
        q->dataChanged(index, index);

        q->beginInsertRows(QModelIndex(), rowMap.count(),
            rowMap.count() + (extraChildren.count() - 1));
    }

    for (int i = 0; i < extraChildren.count(); ++i) {
        rowMap.append(new QVector<int>{extraChildren.at(i)});
    }

    if (!silent) {
        q->endInsertRows();
    }
}

TaskGroupingProxyModel::TaskGroupingProxyModel(QObject *parent) : QAbstractProxyModel(parent)
    , d(new Private(this))
{
}

TaskGroupingProxyModel::~TaskGroupingProxyModel()
{
}

QModelIndex TaskGroupingProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    if (parent.isValid() && row < d->rowMap.at(parent.row())->count()) {
        return createIndex(row, column, d->rowMap.at(parent.row()));
    }

    if (row < d->rowMap.count()) {
        return createIndex(row, column, nullptr);
    }

    return QModelIndex();
}

QModelIndex TaskGroupingProxyModel::parent(const QModelIndex &child) const
{
    if (child.internalPointer() == nullptr) {
        return QModelIndex();
    } else {
        const int parentRow = d->rowMap.indexOf(static_cast<QVector<int> *>(child.internalPointer()));

        if (parentRow != -1) {
            return index(parentRow, 0);
        }

        // If we were asked to find the parent for an internalPointer we can't
        // locate, we have corrupted data: This should not happen.
        Q_ASSERT(parentRow != -1);
    }

    return QModelIndex();
}

QModelIndex TaskGroupingProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.model() != sourceModel()) {
        return QModelIndex();
    }

    for (int i = 0; i < d->rowMap.count(); ++i) {
        const QVector<int> *sourceRows = d->rowMap.at(i);
        const int childIndex = sourceRows->indexOf(sourceIndex.row());
        const QModelIndex parent = index(i, 0);

        if (childIndex == 0) {
            // If the sub-list we found the source row in is larger than 1 (i.e. part
            // of a group, map to the logical child item instead of the parent item
            // the source row also stands in for. The parent is therefore unreachable
            // from mapToSource().
            if (d->isGroup(i)) {
                return index(0, 0, parent);
            // Otherwise map to the top-level item.
            } else {
                return parent;
            }
        } else if (childIndex != -1) {
            return index(childIndex, 0, parent);
        }
    }

    return QModelIndex();
}

QModelIndex TaskGroupingProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid() || proxyIndex.model() != this || !sourceModel()) {
        return QModelIndex();
    }

    const QModelIndex &parent = proxyIndex.parent();

    if (parent.isValid()) {
        if (parent.row() < 0 || parent.row() >= d->rowMap.count()) {
            return QModelIndex();
        }

        return sourceModel()->index(d->rowMap.at(parent.row())->at(proxyIndex.row()), 0);
    } else {
        // Group parents items therefore equate to the first child item; the source
        // row logically appears twice in the proxy.
        // mapFromSource() is not required to handle this well (consider proxies can
        // filter out rows, too) and opts to map to the child item, as the group parent
        // has its Qt::DisplayRole mangled by data(), and it's more useful for trans-
        // lating dataChanged() from the source model.
        return sourceModel()->index(d->rowMap.at(proxyIndex.row())->at(0), 0);
    }

    return QModelIndex();
}

int TaskGroupingProxyModel::rowCount(const QModelIndex &parent) const
{
    if (!sourceModel()) {
        return 0;
    }

    if (parent.isValid() && parent.model() == this) {
        // Don't return row count for top-level item at child row: Group members
        // never have further children of their own.
        if (parent.parent().isValid()) {
            return 0;
        }

        if (parent.row() < 0 || parent.row() >= d->rowMap.count()) {
            return 0;
        }

        const uint rowCount = d->rowMap.at(parent.row())->count();

        // If this sub-list in the map only has one entry, it's a plain item, not
        // parent to a group.
        if (rowCount == 1) {
            return 0;
        } else {
            return rowCount;
        }
    }

    return d->rowMap.count();
}

bool TaskGroupingProxyModel::hasChildren(const QModelIndex &parent) const
{
    if ((parent.model() && parent.model() != this) || !sourceModel()) {
        return false;
    }

    return rowCount(parent);
}

int TaskGroupingProxyModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 1;
}

QVariant TaskGroupingProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
    if (!proxyIndex.isValid() || proxyIndex.model() != this || !sourceModel()) {
        return QVariant();
    }

    const QModelIndex &parent = proxyIndex.parent();
    const bool isGroup = (!parent.isValid() && d->isGroup(proxyIndex.row()));

    // For group parent items, this will map to the first child task.
    const QModelIndex &sourceIndex = mapToSource(proxyIndex);

    if (!sourceIndex.isValid()) {
        return QVariant();
    }

    if (role == AbstractTasksModel::IsGroupable) {
        return !d->isBlacklisted(sourceIndex);
    }

    if (isGroup) {
        // For group parent items, DisplayRole is mapped to AppName of the first child.
        if (role == Qt::DisplayRole) {
            const QString &appName = sourceIndex.data(AbstractTasksModel::AppName).toString();

            // Groups are formed by app id or launcher URL; neither requires
            // AppName to be available. If it's not, fall back to the app id
            /// rather than an empty string.
            if (appName.isEmpty()) {
                return sourceIndex.data(AbstractTasksModel::AppId);
            }

            return appName;
        } else if (role == AbstractTasksModel::WinIdList) {
            QVariantList winIds;

            for (int i = 0; i < rowCount(proxyIndex); ++i) {
                winIds.append(index(i, 0, proxyIndex).data(AbstractTasksModel::WinIdList).toList());
            }

            return winIds;
        } else if (role == AbstractTasksModel::MimeType) {
            return QStringLiteral("windowsystem/multiple-winids");
        } else if (role == AbstractTasksModel::MimeData) {
            // FIXME TODO: Implement.
            return QVariant();
        } else if (role == AbstractTasksModel::IsGroupParent) {
            return true;
        } else if (role == AbstractTasksModel::ChildCount) {
            return rowCount(proxyIndex);
        } else if (role == AbstractTasksModel::IsActive) {
            return d->any(proxyIndex, AbstractTasksModel::IsActive);
        } else if (role == AbstractTasksModel::IsClosable) {
            return d->all(proxyIndex, AbstractTasksModel::IsClosable);
        } else if (role == AbstractTasksModel::IsMovable) {
            // Moving groups makes no sense.
            return false;
        } else if (role == AbstractTasksModel::IsResizable) {
            // Resizing groups makes no sense.
            return false;
        } else if (role == AbstractTasksModel::IsMaximizable) {
            return d->all(proxyIndex, AbstractTasksModel::IsMaximizable);
        } else if (role == AbstractTasksModel::IsMaximized) {
            return d->all(proxyIndex, AbstractTasksModel::IsMaximized);
        } else if (role == AbstractTasksModel::IsMinimizable) {
            return d->all(proxyIndex, AbstractTasksModel::IsMinimizable);
        } else if (role == AbstractTasksModel::IsMinimized) {
            return d->all(proxyIndex, AbstractTasksModel::IsMinimized);
        } else if (role == AbstractTasksModel::IsKeepAbove) {
            return d->all(proxyIndex, AbstractTasksModel::IsKeepAbove);
        } else if (role == AbstractTasksModel::IsKeepBelow) {
            return d->all(proxyIndex, AbstractTasksModel::IsKeepBelow);
        } else if (role == AbstractTasksModel::IsFullScreenable) {
            return d->all(proxyIndex, AbstractTasksModel::IsFullScreenable);
        } else if (role == AbstractTasksModel::IsFullScreen) {
            return d->all(proxyIndex, AbstractTasksModel::IsFullScreen);
        } else if (role == AbstractTasksModel::IsShadeable) {
            return d->all(proxyIndex, AbstractTasksModel::IsShadeable);
        } else if (role == AbstractTasksModel::IsShaded) {
            return d->all(proxyIndex, AbstractTasksModel::IsShaded);
        } else if (role == AbstractTasksModel::IsVirtualDesktopsChangeable) {
            return d->all(proxyIndex, AbstractTasksModel::IsVirtualDesktopsChangeable);
        } else if (role == AbstractTasksModel::VirtualDesktops) {
            QStringList desktops;

            for (int i = 0; i < rowCount(proxyIndex); ++i) {
                desktops.append(index(i, 0, proxyIndex).data(AbstractTasksModel::VirtualDesktops).toStringList());
            }

            desktops.removeDuplicates();
            return desktops;
        } else if (role == AbstractTasksModel::ScreenGeometry) {
            // TODO: Nothing needs this for now and it would add complexity to
            // make it a list; skip it until needed. Once it is, do it similarly
            // to the AbstractTasksModel::VirtualDesktop case.
            return QVariant();
        } else if (role == AbstractTasksModel::Activities) {
            QStringList activities;

            for (int i = 0; i < rowCount(proxyIndex); ++i) {
                activities.append(index(i, 0, proxyIndex).data(AbstractTasksModel::Activities).toStringList());
            }

            activities.removeDuplicates();
            return activities;
        } else if (role == AbstractTasksModel::IsDemandingAttention) {
            return d->any(proxyIndex, AbstractTasksModel::IsDemandingAttention);
        } else if (role == AbstractTasksModel::SkipTaskbar) {
            return d->all(proxyIndex, AbstractTasksModel::SkipTaskbar);
        }
    }

    return sourceIndex.data(role);
}

void TaskGroupingProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel == QAbstractProxyModel::sourceModel()) {
        return;
    }

    beginResetModel();

    if (QAbstractProxyModel::sourceModel()) {
        QAbstractProxyModel::sourceModel()->disconnect(this);
    }

    QAbstractProxyModel::setSourceModel(sourceModel);
    d->abstractTasksSourceModel = dynamic_cast<AbstractTasksModelIface *>(sourceModel);

    if (sourceModel) {
        d->rebuildMap();

        using namespace std::placeholders;
        auto dd = d.data();
        connect(sourceModel, &QSortFilterProxyModel::rowsAboutToBeInserted,
            this, std::bind(&TaskGroupingProxyModel::Private::sourceRowsAboutToBeInserted, dd, _1, _2, _3));
        connect(sourceModel, &QSortFilterProxyModel::rowsInserted,
            this, std::bind(&TaskGroupingProxyModel::Private::sourceRowsInserted, dd, _1, _2, _3));
        connect(sourceModel, &QSortFilterProxyModel::rowsAboutToBeRemoved,
            this, std::bind(&TaskGroupingProxyModel::Private::sourceRowsAboutToBeRemoved, dd, _1, _2, _3));
        connect(sourceModel, &QSortFilterProxyModel::rowsRemoved,
            this, std::bind(&TaskGroupingProxyModel::Private::sourceRowsRemoved, dd, _1, _2, _3));
        connect(sourceModel, &QSortFilterProxyModel::modelAboutToBeReset,
            this, std::bind(&TaskGroupingProxyModel::Private::sourceModelAboutToBeReset, dd));
        connect(sourceModel, &QSortFilterProxyModel::modelReset,
            this, std::bind(&TaskGroupingProxyModel::Private::sourceModelReset, dd));
        connect(sourceModel, &QSortFilterProxyModel::dataChanged,
            this, std::bind(&TaskGroupingProxyModel::Private::sourceDataChanged, dd, _1, _2, _3));
    } else {
        d->rowMap.clear();
    }

    endResetModel();
}

TasksModel::GroupMode TaskGroupingProxyModel::groupMode() const
{
    return d->groupMode;
}

void TaskGroupingProxyModel::setGroupMode(TasksModel::GroupMode mode)
{
    if (d->groupMode != mode) {

        d->groupMode = mode;

        d->checkGrouping();

        emit groupModeChanged();
    }
}

bool TaskGroupingProxyModel::groupDemandingAttention() const
{
    return d->groupDemandingAttention;
}

void TaskGroupingProxyModel::setGroupDemandingAttention(bool group)
{
    if (d->groupDemandingAttention != group) {

        d->groupDemandingAttention = group;

        d->checkGrouping();

        emit groupDemandingAttentionChanged();
    }
}

int TaskGroupingProxyModel::windowTasksThreshold() const
{
    return d->windowTasksThreshold;
}

void TaskGroupingProxyModel::setWindowTasksThreshold(int threshold)
{
    if (d->windowTasksThreshold != threshold) {
        d->windowTasksThreshold = threshold;

        d->checkGrouping();

        emit windowTasksThresholdChanged();
    }
}

QStringList TaskGroupingProxyModel::blacklistedAppIds() const
{
    return d->blacklistedAppIds.values();
}

void TaskGroupingProxyModel::setBlacklistedAppIds(const QStringList &list)
{
    const QSet<QString> &set = QSet<QString>(list.cbegin(), list.cend());

    if (d->blacklistedAppIds != set) {
        d->blacklistedAppIds = set;

        // checkGrouping() will gather and group up what's newly-allowed under the changed
        // blacklist.
        d->checkGrouping();

        // Now break apart what we need to.
        for (int i = (d->rowMap.count() - 1); i >= 0; --i) {
            if (d->isGroup(i)) {
                const QModelIndex &groupRep = index(i, 0);

                if (set.contains(groupRep.data(AbstractTasksModel::AppId).toString())) {
                    d->breakGroupFor(groupRep); // Safe since we're iterating backwards.
                }
            }
        }

        emit blacklistedAppIdsChanged();
    }
}

QStringList TaskGroupingProxyModel::blacklistedLauncherUrls() const
{
    return d->blacklistedLauncherUrls.values();
}

void TaskGroupingProxyModel::setBlacklistedLauncherUrls(const QStringList &list)
{
    const QSet<QString> &set = QSet<QString>(list.cbegin(), list.cend());

    if (d->blacklistedLauncherUrls != set) {
        d->blacklistedLauncherUrls = set;

        // checkGrouping() will gather and group up what's newly-allowed under the changed
        // blacklist.
        d->checkGrouping();

        // Now break apart what we need to.
        for (int i = (d->rowMap.count() - 1); i >= 0; --i) {
            if (d->isGroup(i)) {
                const QModelIndex &groupRep = index(i, 0);
                const QUrl &launcherUrl = groupRep.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl();
                const QString &launcherUrlString = launcherUrl.toString(QUrl::RemoveQuery | QUrl::RemoveQuery);

                if (set.contains(launcherUrlString)) {
                    d->breakGroupFor(groupRep); // Safe since we're iterating backwards.
                }
            }
        }

        emit blacklistedLauncherUrlsChanged();
    }
}

void TaskGroupingProxyModel::requestActivate(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestActivate(mapToSource(index));
    }
}

void TaskGroupingProxyModel::requestNewInstance(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    d->abstractTasksSourceModel->requestNewInstance(mapToSource(index));
}

void TaskGroupingProxyModel::requestOpenUrls(const QModelIndex &index, const QList<QUrl> &urls)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    d->abstractTasksSourceModel->requestOpenUrls(mapToSource(index), urls);
}

void TaskGroupingProxyModel::requestClose(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestClose(mapToSource(index));
    } else {
        const int row = index.row();

        for (int i = (rowCount(index) - 1); i >= 1; --i) {
            const QModelIndex &sourceChild = mapToSource(this->index(i, 0, index));
            d->abstractTasksSourceModel->requestClose(sourceChild);
        }

        d->abstractTasksSourceModel->requestClose(mapToSource(TaskGroupingProxyModel::index(row, 0)));
    }
}

void TaskGroupingProxyModel::requestMove(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestMove(mapToSource(index));
    }
}

void TaskGroupingProxyModel::requestResize(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestResize(mapToSource(index));
    }
}

void TaskGroupingProxyModel::requestToggleMinimized(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestToggleMinimized(mapToSource(index));
    } else {
        const bool goalState = !index.data(AbstractTasksModel::IsMinimized).toBool();

         for (int i = 0; i < rowCount(index); ++i) {
             const QModelIndex &child = this->index(i, 0, index);

             if (child.data(AbstractTasksModel::IsMinimized).toBool() != goalState) {
                 d->abstractTasksSourceModel->requestToggleMinimized(mapToSource(child));
             }
         }
    }
}

void TaskGroupingProxyModel::requestToggleMaximized(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestToggleMaximized(mapToSource(index));
    } else {
        const bool goalState = !index.data(AbstractTasksModel::IsMaximized).toBool();

        QModelIndexList inStackingOrder;

        for (int i = 0; i < rowCount(index); ++i) {
            const QModelIndex &child = this->index(i, 0, index);

            if (child.data(AbstractTasksModel::IsMaximized).toBool() != goalState) {
                inStackingOrder << mapToSource(child);
            }
        }

        std::sort(inStackingOrder.begin(), inStackingOrder.end(),
            [](const QModelIndex &a, const QModelIndex &b)  {
                return (a.data(AbstractTasksModel::StackingOrder).toInt()
                    < b.data(AbstractTasksModel::StackingOrder).toInt());
            }
        );

        for (const QModelIndex &sourceChild : inStackingOrder) {
            d->abstractTasksSourceModel->requestToggleMaximized(sourceChild);
        }
    }
}

void TaskGroupingProxyModel::requestToggleKeepAbove(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestToggleKeepAbove(mapToSource(index));
    } else {
        const bool goalState = !index.data(AbstractTasksModel::IsKeepAbove).toBool();

         for (int i = 0; i < rowCount(index); ++i) {
             const QModelIndex &child = this->index(i, 0, index);

             if (child.data(AbstractTasksModel::IsKeepAbove).toBool() != goalState) {
                 d->abstractTasksSourceModel->requestToggleKeepAbove(mapToSource(child));
             }
         }
    }
}

void TaskGroupingProxyModel::requestToggleKeepBelow(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestToggleKeepBelow(mapToSource(index));
    } else {
        const bool goalState = !index.data(AbstractTasksModel::IsKeepBelow).toBool();

         for (int i = 0; i < rowCount(index); ++i) {
             const QModelIndex &child = this->index(i, 0, index);

             if (child.data(AbstractTasksModel::IsKeepBelow).toBool() != goalState) {
                 d->abstractTasksSourceModel->requestToggleKeepBelow(mapToSource(child));
             }
         }
    }
}

void TaskGroupingProxyModel::requestToggleFullScreen(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestToggleFullScreen(mapToSource(index));
    } else {
        const bool goalState = !index.data(AbstractTasksModel::IsFullScreen).toBool();

         for (int i = 0; i < rowCount(index); ++i) {
             const QModelIndex &child = this->index(i, 0, index);

             if (child.data(AbstractTasksModel::IsFullScreen).toBool() != goalState) {
                 d->abstractTasksSourceModel->requestToggleFullScreen(mapToSource(child));
             }
         }
    }
}

void TaskGroupingProxyModel::requestToggleShaded(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestToggleShaded(mapToSource(index));
    } else {
        const bool goalState = !index.data(AbstractTasksModel::IsShaded).toBool();

         for (int i = 0; i < rowCount(index); ++i) {
             const QModelIndex &child = this->index(i, 0, index);

             if (child.data(AbstractTasksModel::IsShaded).toBool() != goalState) {
                 d->abstractTasksSourceModel->requestToggleShaded(mapToSource(child));
             }
         }
    }
}

void TaskGroupingProxyModel::requestVirtualDesktops(const QModelIndex &index, const QVariantList &desktops)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestVirtualDesktops(mapToSource(index), desktops);
    } else {
        QVector<QModelIndex> groupChildren;

        const int childCount = rowCount(index);

        groupChildren.reserve(childCount);

        for (int i = (childCount - 1); i >= 0; --i) {
            groupChildren.append(mapToSource(this->index(i, 0, index)));
        }

        for (const QModelIndex &idx : groupChildren) {
            d->abstractTasksSourceModel->requestVirtualDesktops(idx, desktops);
        }
    }
}

void TaskGroupingProxyModel::requestNewVirtualDesktop(const QModelIndex &index)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestNewVirtualDesktop(mapToSource(index));
    } else {
        QVector<QModelIndex> groupChildren;

        const int childCount = rowCount(index);

        groupChildren.reserve(childCount);

        for (int i = (childCount - 1); i >= 0; --i) {
            groupChildren.append(mapToSource(this->index(i, 0, index)));
        }

        for (const QModelIndex &idx : groupChildren) {
            d->abstractTasksSourceModel->requestNewVirtualDesktop(idx);
        }
    }
}

void TaskGroupingProxyModel::requestActivities(const QModelIndex &index, const QStringList &activities)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestActivities(mapToSource(index), activities);
    } else {
        QVector<QModelIndex> groupChildren;

        const int childCount = rowCount(index);

        groupChildren.reserve(childCount);

        for (int i = (childCount - 1); i >= 0; --i) {
            groupChildren.append(mapToSource(this->index(i, 0, index)));
        }

        for (const QModelIndex &idx : groupChildren) {
            d->abstractTasksSourceModel->requestActivities(idx, activities);
        }
    }
}

void TaskGroupingProxyModel::requestPublishDelegateGeometry(const QModelIndex &index, const QRect &geometry, QObject *delegate)
{
    if (!d->abstractTasksSourceModel || !index.isValid() || index.model() != this) {
        return;
    }

    if (index.parent().isValid() || !d->isGroup(index.row())) {
        d->abstractTasksSourceModel->requestPublishDelegateGeometry(mapToSource(index),
            geometry, delegate);
    } else {
        for (int i = 0; i < rowCount(index); ++i) {
            d->abstractTasksSourceModel->requestPublishDelegateGeometry(mapToSource(this->index(i, 0, index)),
                geometry, delegate);
        }
    }
}

void TaskGroupingProxyModel::requestToggleGrouping(const QModelIndex &index)
{
    const QString &appId = index.data(AbstractTasksModel::AppId).toString();
    const QUrl &launcherUrl = index.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl();
    const QString &launcherUrlString = launcherUrl.toString(QUrl::RemoveQuery | QUrl::RemoveQuery);

    if (d->blacklistedAppIds.contains(appId) || d->blacklistedLauncherUrls.contains(launcherUrlString)) {
        d->blacklistedAppIds.remove(appId);
        d->blacklistedLauncherUrls.remove(launcherUrlString);

        if (d->groupMode != TasksModel::GroupDisabled) {
            d->formGroupFor(index.parent().isValid() ? index.parent() : index);
        }
    } else {
        d->blacklistedAppIds.insert(appId);
        d->blacklistedLauncherUrls.insert(launcherUrlString);

        if (d->groupMode != TasksModel::GroupDisabled) {
            d->breakGroupFor(index.parent().isValid() ? index.parent() : index);
        }
    }

    // Update IsGroupable data role for all relevant top-level items. We don't need to update
    // for group members since they've just been inserted -- it's logically impossible to
    // toggle grouping _on_ from a group member.
    for (int i = 0; i < d->rowMap.count(); ++i) {
        if (!d->isGroup(i)) {
            const QModelIndex &idx = TaskGroupingProxyModel::index(i, 0);

            if (idx.data(AbstractTasksModel::AppId).toString() == appId
                || launcherUrlsMatch(idx.data(AbstractTasksModel::LauncherUrlWithoutIcon).toUrl(), launcherUrl,
                IgnoreQueryItems)) {
                dataChanged(idx, idx, QVector<int>{AbstractTasksModel::IsGroupable});
            }
        }
    }

    emit blacklistedAppIdsChanged();
    emit blacklistedLauncherUrlsChanged();
}

}

#include "moc_taskgroupingproxymodel.cpp"
