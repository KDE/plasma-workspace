/*
 * Copyright 2016 Eike Hein <hein@kde.org>
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

#include "notificationgroupingproxymodel_p.h"

#include <QDateTime>

#include "notifications.h"

using namespace NotificationManager;

NotificationGroupingProxyModel::NotificationGroupingProxyModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

NotificationGroupingProxyModel::~NotificationGroupingProxyModel() = default;

bool NotificationGroupingProxyModel::appsMatch(const QModelIndex &a, const QModelIndex &b) const
{
    const QString aName = a.data(Notifications::ApplicationNameRole).toString();
    const QString bName = b.data(Notifications::ApplicationNameRole).toString();

    const QString aDesktopEntry = a.data(Notifications::DesktopEntryRole).toString();
    const QString bDesktopEntry = b.data(Notifications::DesktopEntryRole).toString();

    const QString aOriginName = a.data(Notifications::OriginNameRole).toString();
    const QString bOriginName = b.data(Notifications::OriginNameRole).toString();

    return !aName.isEmpty() && aName == bName && aDesktopEntry == bDesktopEntry && aOriginName == bOriginName;
}

bool NotificationGroupingProxyModel::isGroup(int row) const
{
    if (row < 0 || row >= rowMap.count()) {
        return false;
    }

    return (rowMap.at(row)->count() > 1);
}

bool NotificationGroupingProxyModel::tryToGroup(const QModelIndex &sourceIndex, bool silent)
{
    // Meat of the matter: Try to add this source row to a sub-list with source rows
    // associated with the same application.
    for (int i = 0; i < rowMap.count(); ++i) {
        const QModelIndex &groupRep = sourceModel()->index(rowMap.at(i)->constFirst(), 0);

        // Don't match a row with itself.
        if (sourceIndex == groupRep) {
            continue;
        }

        if (appsMatch(sourceIndex, groupRep)) {
            const QModelIndex parent = index(i, 0);

            if (!silent) {
                const int newIndex = rowMap.at(i)->count();

                if (newIndex == 1) {
                    beginInsertRows(parent, 0, 1);
                } else {
                    beginInsertRows(parent, newIndex, newIndex);
                }
            }

            rowMap[i]->append(sourceIndex.row());

            if (!silent) {
                endInsertRows();

                dataChanged(parent, parent);
            }

            return true;
        }
    }

    return false;
}

void NotificationGroupingProxyModel::adjustMap(int anchor, int delta)
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

void NotificationGroupingProxyModel::rebuildMap()
{
    qDeleteAll(rowMap);
    rowMap.clear();

    const int rows = sourceModel()->rowCount();

    rowMap.reserve(rows);

    for (int i = 0; i < rows; ++i) {
        rowMap.append(new QVector<int>{i});
    }

    checkGrouping(true /* silent */);
}

void NotificationGroupingProxyModel::checkGrouping(bool silent)
{
    for (int i = (rowMap.count()) - 1; i >= 0; --i) {
        if (isGroup(i)) {
            continue;
        }

        // FIXME support skip grouping hint, maybe?
        // The new grouping keeps every notification separate, still, so perhaps we don't need to

        if (tryToGroup(sourceModel()->index(rowMap.at(i)->constFirst(), 0), silent)) {
            beginRemoveRows(QModelIndex(), i, i);
            delete rowMap.takeAt(i); // Safe since we're iterating backwards.
            endRemoveRows();
        }
    }
}

void NotificationGroupingProxyModel::formGroupFor(const QModelIndex &index)
{
    // Already in group or a group.
    if (index.parent().isValid() || isGroup(index.row())) {
        return;
    }

    // We need to grab a source index as we may invalidate the index passed
    // in through grouping.
    const QModelIndex &sourceTarget = mapToSource(index);

    for (int i = (rowMap.count() - 1); i >= 0; --i) {
        const QModelIndex &sourceIndex = sourceModel()->index(rowMap.at(i)->constFirst(), 0);

        if (!appsMatch(sourceTarget, sourceIndex)) {
            continue;
        }

        if (tryToGroup(sourceIndex)) {
            beginRemoveRows(QModelIndex(), i, i);
            delete rowMap.takeAt(i); // Safe since we're iterating backwards.
            endRemoveRows();
        }
    }
}

void NotificationGroupingProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    if (sourceModel == QAbstractProxyModel::sourceModel()) {
        return;
    }

    beginResetModel();

    if (QAbstractProxyModel::sourceModel()) {
        QAbstractProxyModel::sourceModel()->disconnect(this);
    }

    QAbstractProxyModel::setSourceModel(sourceModel);

    if (sourceModel) {
        rebuildMap();

        // FIXME move this stuff into separate slot methods

        connect(sourceModel, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &parent, int start, int end) {
            if (parent.isValid()) {
                return;
            }

            adjustMap(start, (end - start) + 1);

            for (int i = start; i <= end; ++i) {
                if (!tryToGroup(this->sourceModel()->index(i, 0))) {
                    beginInsertRows(QModelIndex(), rowMap.count(), rowMap.count());
                    rowMap.append(new QVector<int>{i});
                    endInsertRows();
                }
            }

            checkGrouping();
        });

        connect(sourceModel, &QAbstractItemModel::rowsAboutToBeRemoved, this, [this](const QModelIndex &parent, int first, int last) {
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
                            beginRemoveRows(QModelIndex(), j, j);
                            delete rowMap.takeAt(j);
                            endRemoveRows();
                            // Dissolve group.
                        } else if (sourceRows->count() == 2) {
                            const QModelIndex parent = index(j, 0);
                            beginRemoveRows(parent, 0, 1);
                            rowMap[j]->remove(mapIndex);
                            endRemoveRows();

                            // We're no longer a group parent.
                            dataChanged(parent, parent);
                            // Remove group member.
                        } else {
                            const QModelIndex parent = index(j, 0);
                            beginRemoveRows(parent, mapIndex, mapIndex);
                            rowMap[j]->remove(mapIndex);
                            endRemoveRows();

                            // Various roles of the parent evaluate child data, and the
                            // child list has changed.
                            dataChanged(parent, parent);

                            // Signal children count change for all other items in the group.
                            emit dataChanged(index(0, 0, parent), index(rowMap.count() - 1, 0, parent), {Notifications::GroupChildrenCountRole});
                        }

                        break;
                    }
                }
            }
        });

        connect(sourceModel, &QAbstractItemModel::rowsRemoved, this, [this](const QModelIndex &parent, int start, int end) {
            if (parent.isValid()) {
                return;
            }

            adjustMap(start + 1, -((end - start) + 1));

            checkGrouping();
        });

        connect(sourceModel, &QAbstractItemModel::modelAboutToBeReset, this, &NotificationGroupingProxyModel::beginResetModel);
        connect(sourceModel, &QAbstractItemModel::modelReset, this, [this] {
            rebuildMap();
            endResetModel();
        });

        connect(sourceModel,
                &QAbstractItemModel::dataChanged,
                this,
                [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles) {
                    for (int i = topLeft.row(); i <= bottomRight.row(); ++i) {
                        const QModelIndex &sourceIndex = this->sourceModel()->index(i, 0);
                        QModelIndex proxyIndex = mapFromSource(sourceIndex);

                        if (!proxyIndex.isValid()) {
                            return;
                        }

                        const QModelIndex parent = proxyIndex.parent();

                        // If a child item changes, its parent may need an update as well as many of
                        // the data roles evaluate child data. See data().
                        // TODO: Some roles do not need to bubble up as they fall through to the first
                        // child in data(); it _might_ be worth adding constraints here later.
                        if (parent.isValid()) {
                            dataChanged(parent, parent, roles);
                        }

                        dataChanged(proxyIndex, proxyIndex, roles);
                    }
                });
    }

    endResetModel();
}

QModelIndex NotificationGroupingProxyModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || column != 0) {
        return QModelIndex();
    }

    if (parent.isValid() && row < rowMap.at(parent.row())->count()) {
        return createIndex(row, column, rowMap.at(parent.row()));
    }

    if (row < rowMap.count()) {
        return createIndex(row, column, nullptr);
    }

    return QModelIndex();
}

QModelIndex NotificationGroupingProxyModel::parent(const QModelIndex &child) const
{
    if (child.internalPointer() == nullptr) {
        return QModelIndex();
    } else {
        const int parentRow = rowMap.indexOf(static_cast<QVector<int> *>(child.internalPointer()));

        if (parentRow != -1) {
            return index(parentRow, 0);
        }

        // If we were asked to find the parent for an internalPointer we can't
        // locate, we have corrupted data: This should not happen.
        Q_ASSERT(parentRow != -1);
    }

    return QModelIndex();
}

QModelIndex NotificationGroupingProxyModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.model() != sourceModel()) {
        return QModelIndex();
    }

    for (int i = 0; i < rowMap.count(); ++i) {
        const QVector<int> *sourceRows = rowMap.at(i);
        const int childIndex = sourceRows->indexOf(sourceIndex.row());
        const QModelIndex parent = index(i, 0);

        if (childIndex == 0) {
            // If the sub-list we found the source row in is larger than 1 (i.e. part
            // of a group, map to the logical child item instead of the parent item
            // the source row also stands in for. The parent is therefore unreachable
            // from mapToSource().
            if (isGroup(i)) {
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

QModelIndex NotificationGroupingProxyModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid() || proxyIndex.model() != this || !sourceModel()) {
        return QModelIndex();
    }

    const QModelIndex &parent = proxyIndex.parent();

    if (parent.isValid()) {
        if (parent.row() < 0 || parent.row() >= rowMap.count()) {
            return QModelIndex();
        }

        return sourceModel()->index(rowMap.at(parent.row())->at(proxyIndex.row()), 0);
    } else {
        // Group parents items therefore equate to the first child item; the source
        // row logically appears twice in the proxy.
        // mapFromSource() is not required to handle this well (consider proxies can
        // filter out rows, too) and opts to map to the child item, as the group parent
        // has its Qt::DisplayRole mangled by data(), and it's more useful for trans-
        // lating dataChanged() from the source model.
        // NOTE we changed that to be last
        if (rowMap.isEmpty()) { // FIXME
            // How can this happen? (happens when closing a group)
            return QModelIndex();
        }
        return sourceModel()->index(rowMap.at(proxyIndex.row())->constLast(), 0);
    }

    return QModelIndex();
}

int NotificationGroupingProxyModel::rowCount(const QModelIndex &parent) const
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

        if (parent.row() < 0 || parent.row() >= rowMap.count()) {
            return 0;
        }

        const int rowCount = rowMap.at(parent.row())->count();

        // If this sub-list in the map only has one entry, it's a plain item, not
        // parent to a group.
        if (rowCount == 1) {
            return 0;
        } else {
            return rowCount;
        }
    }

    return rowMap.count();
}

bool NotificationGroupingProxyModel::hasChildren(const QModelIndex &parent) const
{
    if ((parent.model() && parent.model() != this) || !sourceModel()) {
        return false;
    }

    return rowCount(parent);
}

int NotificationGroupingProxyModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 1;
}

QVariant NotificationGroupingProxyModel::data(const QModelIndex &proxyIndex, int role) const
{
    if (!proxyIndex.isValid() || proxyIndex.model() != this || !sourceModel()) {
        return QVariant();
    }

    const QModelIndex &parent = proxyIndex.parent();
    const bool isGroup = (!parent.isValid() && this->isGroup(proxyIndex.row()));

    // For group parent items, this will map to the last child task.
    const QModelIndex &sourceIndex = mapToSource(proxyIndex);

    if (!sourceIndex.isValid()) {
        return QVariant();
    }

    if (isGroup) {
        switch (role) {
        case Notifications::IsGroupRole:
            return true;
        case Notifications::GroupChildrenCountRole:
            return rowCount(proxyIndex);
        case Notifications::IsInGroupRole:
            return false;

        // Combine all notifications into one for some basic grouping
        case Notifications::BodyRole: {
            QString body;
            for (int i = 0; i < rowCount(proxyIndex); ++i) {
                const QString stringData = index(i, 0, proxyIndex).data(role).toString();
                if (!stringData.isEmpty()) {
                    if (!body.isEmpty()) {
                        body.append(QLatin1String("<br>"));
                    }
                    body.append(stringData);
                }
            }
            return body;
        }

        case Notifications::DesktopEntryRole:
        case Notifications::NotifyRcNameRole:
        case Notifications::OriginNameRole:
            for (int i = 0; i < rowCount(proxyIndex); ++i) {
                const QString stringData = index(i, 0, proxyIndex).data(role).toString();
                if (!stringData.isEmpty()) {
                    return stringData;
                }
            }
            return QString();

        case Notifications::ConfigurableRole: // if there is any configurable child item
            for (int i = 0; i < rowCount(proxyIndex); ++i) {
                if (index(i, 0, proxyIndex).data(Notifications::ConfigurableRole).toBool()) {
                    return true;
                }
            }
            return false;

        case Notifications::ClosableRole: // if there is any closable child item
            for (int i = 0; i < rowCount(proxyIndex); ++i) {
                if (index(i, 0, proxyIndex).data(Notifications::ClosableRole).toBool()) {
                    return true;
                }
            }
            return false;
        }
    } else {
        switch (role) {
        case Notifications::IsGroupRole:
            return false;
        // So a notification knows with how many other items it is in a group
        case Notifications::GroupChildrenCountRole:
            if (proxyIndex.parent().isValid()) {
                return rowCount(proxyIndex.parent());
            }
            break;
        case Notifications::IsInGroupRole:
            return parent.isValid();
        }
    }

    return sourceIndex.data(role);
}
