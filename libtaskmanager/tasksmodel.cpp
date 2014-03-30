/*****************************************************************

Copyright 2010 Aaron Seigo <aseigo@kde.org>
Copyright 2012-2013 Eike Hein <hein@kde.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*****************************************************************/

#include "tasksmodel.h"

#include <QMetaEnum>
#include <QMimeData>

#include "groupmanager.h"
#include "taskgroup.h"

namespace TaskManager
{

class TasksModelPrivate
{
public:
    TasksModelPrivate(TasksModel *model, GroupManager *gm);

    void populateModel();
    void populate(const QModelIndex &parent, TaskGroup *group);
    void itemAboutToBeAdded(AbstractGroupableItem *item, int index);
    void itemAdded(AbstractGroupableItem *item);
    void itemAboutToBeRemoved(AbstractGroupableItem *item);
    void itemRemoved(AbstractGroupableItem *item);
    void itemAboutToMove(AbstractGroupableItem *item, int currentIndex, int newIndex);
    void itemMoved(AbstractGroupableItem *item);
    void itemChanged(::TaskManager::TaskChanges changes);

    int indexOf(AbstractGroupableItem *item);

    TasksModel *q;
    QWeakPointer<GroupManager> groupManager;
    TaskGroup *rootGroup;
};

TasksModel::TasksModel(GroupManager *groupManager, QObject *parent)
    : QAbstractItemModel(parent),
      d(new TasksModelPrivate(this, groupManager))
{
    // set the role names based on the values of the DisplayRoles enum for the sake of QML
    QHash<int, QByteArray> roles;
    roles.insert(Qt::DisplayRole, "DisplayRole");
    roles.insert(Qt::DecorationRole, "DecorationRole");
    QMetaEnum e = metaObject()->enumerator(metaObject()->indexOfEnumerator("DisplayRoles"));
    for (int i = 0; i < e.keyCount(); ++i) {
        roles.insert(e.value(i), e.key(i));
    }
    setRoleNames(roles);

    if (groupManager) {
        connect(groupManager, SIGNAL(reload()), this, SLOT(populateModel()));
    }

    connect(this, SIGNAL(rowsInserted(QModelIndex, int, int)), this, SIGNAL(countChanged()));
    connect(this, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SIGNAL(countChanged()));
}

TasksModel::~TasksModel()
{
    delete d;
}

QVariant TasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    AbstractGroupableItem *item = static_cast<AbstractGroupableItem *>(index.internalPointer());
    if (!item) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return item->name();
    } else if (role == Qt::DecorationRole) {
        return item->icon();
    } else if (role == TasksModel::Id) {
        return item->id();
    } else if (role == TasksModel::GenericName) {
        return item->genericName();
    } else if (role == TasksModel::IsStartup) {
        return item->isStartupItem();
    } else if (role == TasksModel::IsLauncher) {
        return item->itemType() == LauncherItemType;
    } else if (role == TasksModel::OnAllDesktops) {
        return item->isOnAllDesktops();
    } else if (role == TasksModel::Desktop) {
        return item->desktop();
    } else if (role == TasksModel::DesktopName) {
        return KWindowSystem::desktopName(item->desktop());
    } else if (role == TasksModel::OnAllActivities) {
        // FIXME: 2ac45a07d6 should have put isOnAllActivities() & co
        // into AbstractGroupableItem to avoid this scaffolding; make
        // that happen.
        if (item->itemType() == TaskItemType) {
            TaskItem *taskItem = static_cast<TaskItem *>(item);

            if (taskItem->task()) {
                return taskItem->task()->isOnAllActivities();
            }
        }

        return false;
    } else if (role == TasksModel::ActivityNames) {
        QVariantList activities;

        if (item->itemType() == TaskItemType) {
            foreach(const QString &activity, static_cast<TaskItem *>(item)->activityNames()) {
                activities << activity;
            }
        }

        return activities;
    } else if (role == TasksModel::OtherActivityNames) {
        QVariantList activities;

        if (item->itemType() == TaskItemType) {
            foreach(const QString &activity, static_cast<TaskItem *>(item)->activityNames(false)) {
                activities << activity;
            }
        }

        return activities;
    } else if (role == TasksModel::Shaded) {
        return item->isShaded();
    } else if (role == TasksModel::Maximized) {
        return item->isMaximized();
    } else if (role == TasksModel::Minimized) {
        return item->isMinimized();
    } else if (role == TasksModel::FullScreen) {
        return item->isFullScreen();
    } else if (role == TasksModel::BelowOthers) {
        return item->isKeptBelowOthers();
    } else if (role == TasksModel::AlwaysOnTop) {
        return item->isAlwaysOnTop();
    } else if (role == TasksModel::Active) {
        return item->isActive();
    } else if (role == TasksModel::DemandsAttention) {
        return item->demandsAttention();
    } else if (role == TasksModel::LauncherUrl) {
        return QUrl(item->launcherUrl());
    } else if (role == TasksModel::WindowList) {
        QVariantList windows;

        foreach(const qulonglong winId, item->winIds()) {
            windows << winId;
        }

        return windows;
    } else if (role == TasksModel::MimeType) {
        if (item->itemType() == TaskItemType) {
            return Task::mimetype();
        } else if (item->itemType() == GroupItemType) {
            return Task::groupMimetype();
        }
    } else if (role == TasksModel::MimeData) {
        QMimeData mimeData;

        item->addMimeData(&mimeData);

        if (item->itemType() == TaskItemType) {
            return QVariant::fromValue(mimeData.data(Task::mimetype()));
        } else if (item->itemType() == GroupItemType) {
            return QVariant::fromValue(mimeData.data(Task::groupMimetype()));
        }
    }

    return QVariant();
}

int TasksModel::activeTaskId(TaskGroup *group) const
{
    group = group ? group : d->rootGroup;
    int id = -1;

    foreach (AbstractGroupableItem *item, group->members()) {
        if (item->itemType() == TaskItemType) {
            TaskItem *taskItem = static_cast<TaskItem *>(item);

            if (taskItem && taskItem->task() && taskItem->task()->isActive()) {
                id = item->id();

                break;
            }
        } else if (item->itemType() == GroupItemType) {
            id = activeTaskId(static_cast<TaskGroup *>(item));

            if (id > -1) {
                break;
            }
        }
    }

    return id;
}

QVariant TasksModel::taskIdList(const QModelIndex& parent, bool recursive) const
{
    QVariantList taskIds;

    TaskGroup* parentGroup = d->rootGroup;

    AbstractGroupableItem *parentItem = static_cast<AbstractGroupableItem *>(parent.internalPointer());

    if (parent.isValid() && parentItem->itemType() == GroupItemType) {
        parentGroup = static_cast<TaskGroup *>(parentItem);
    }

    foreach (AbstractGroupableItem *item, parentGroup->members()) {
        if (item->itemType() == TaskItemType) {
            taskIds << item->id();
        } else if (recursive) {
            if (item->itemType() == GroupItemType)
            {
                foreach(AbstractGroupableItem *subItem, static_cast<TaskGroup *>(item)->members()) {
                    taskIds << subItem->id();
                }
            }
        }
    }

    return taskIds;
}

QModelIndex TasksModel::index(int row, int column, const QModelIndex &parent) const
{
    GroupManager *gm = d->groupManager.data();
    if (!gm || row < 0 || column < 0 || column > 0) {
        return QModelIndex();
    }

    //kDebug() << "asking for" << row << column;

    TaskGroup *group = 0;
    if (parent.isValid()) {
        AbstractGroupableItem *item = static_cast<AbstractGroupableItem *>(parent.internalPointer());
        if (item && item->itemType() == GroupItemType) {
            group = static_cast<TaskGroup *>(item);
        }
    } else {
        group = gm->rootGroup();
    }

    if (!group || row >= group->members().count()) {
        return QModelIndex();
    }

    return createIndex(row, column, group->members().at(row));
}

QModelIndex TasksModel::parent(const QModelIndex &idx) const
{
    GroupManager *gm = d->groupManager.data();
    if (!gm) {
        return QModelIndex();
    }

    AbstractGroupableItem *item = static_cast<AbstractGroupableItem *>(idx.internalPointer());
    if (!item) {
        return QModelIndex();
    }

    TaskGroup *group = item->parentGroup();
    if (!group || group == gm->rootGroup() || !group->parentGroup()) {
        return QModelIndex();
    }

    int row = 0;
    bool found = false;
    TaskGroup *grandparent = group->parentGroup();
    while (row < grandparent->members().count()) {
        if (group == grandparent->members().at(row)) {
            found = true;
            break;
        }

        ++row;
    }

    if (!found) {
        return QModelIndex();
    }

    return createIndex(row, idx.column(), group);
}

int TasksModel::columnCount(const QModelIndex &) const
{
    return 1;
}

int TasksModel::rowCount(const QModelIndex &parent) const
{
    if (!d->rootGroup) {
        return 0;
    }

    if (!parent.isValid()) {
        return d->rootGroup->members().count();
    }

    AbstractGroupableItem *item = static_cast<AbstractGroupableItem *>(parent.internalPointer());

    if (item && item->itemType() == GroupItemType) {
        TaskGroup *group = static_cast<TaskGroup *>(item);
        return group->members().count();
    }

    return 0;
}

int TasksModel::launcherCount() const
{
    if (!d->rootGroup) {
        return 0;
    }

    int launcherCount = 0;

    foreach (AbstractGroupableItem *item, d->rootGroup->members()) {
        if (item->itemType() == LauncherItemType) {
            ++launcherCount;
        }
    }

    return launcherCount;
}

TasksModelPrivate::TasksModelPrivate(TasksModel *model, GroupManager *gm)
    : q(model),
      groupManager(gm),
      rootGroup(0)
{
}

void TasksModelPrivate::populateModel()
{
    GroupManager *gm = groupManager.data();

    if (!gm) {
        rootGroup = 0;
        return;
    }


    if (rootGroup != gm->rootGroup()) {
        if (rootGroup) {
            QObject::disconnect(rootGroup, 0, q, 0);
        }

        rootGroup = gm->rootGroup();
    }

    q->beginResetModel();
    const QModelIndex idx;
    populate(idx, rootGroup);
    q->endResetModel();
}

void TasksModelPrivate::populate(const QModelIndex &parent, TaskGroup *group)
{
    QObject::connect(group, SIGNAL(itemAboutToBeAdded(AbstractGroupableItem*, int)),
                     q, SLOT(itemAboutToBeAdded(AbstractGroupableItem*, int)),
                     Qt::UniqueConnection);
    QObject::connect(group, SIGNAL(itemAdded(AbstractGroupableItem*)),
                     q, SLOT(itemAdded(AbstractGroupableItem*)),
                     Qt::UniqueConnection);
    QObject::connect(group, SIGNAL(itemAboutToBeRemoved(AbstractGroupableItem*)),
                     q, SLOT(itemAboutToBeRemoved(AbstractGroupableItem*)),
                     Qt::UniqueConnection);
    QObject::connect(group, SIGNAL(itemRemoved(AbstractGroupableItem*)),
                     q, SLOT(itemRemoved(AbstractGroupableItem*)),
                     Qt::UniqueConnection);
    QObject::connect(group, SIGNAL(itemAboutToMove(AbstractGroupableItem*, int, int)),
                     q, SLOT(itemAboutToMove(AbstractGroupableItem*, int, int)),
                     Qt::UniqueConnection);
    QObject::connect(group, SIGNAL(itemPositionChanged(AbstractGroupableItem*)),
                     q, SLOT(itemMoved(AbstractGroupableItem*)),
                     Qt::UniqueConnection);

    if (group->members().isEmpty()) {
        return;
    }

    typedef QPair<QModelIndex, TaskGroup *> idxGroupPair;
    QList<idxGroupPair> childGroups;

    int i = 0;
    foreach (AbstractGroupableItem * item,  group->members()) {
        if (item->itemType() == GroupItemType) {
            QModelIndex idx(q->index(i, 0, parent));
            childGroups << idxGroupPair(idx, static_cast<TaskGroup *>(item));
        }

        QObject::connect(item, SIGNAL(changed(::TaskManager::TaskChanges)),
                         q, SLOT(itemChanged(::TaskManager::TaskChanges)),
                         Qt::UniqueConnection);
        ++i;
    }

    foreach (const idxGroupPair & pair, childGroups) {
        populate(pair.first, pair.second);
    }
}

int TasksModelPrivate::indexOf(AbstractGroupableItem *item)
{
    Q_ASSERT(item != rootGroup);
    int row = 0;

    if (!item->parentGroup())
        return -1;

    foreach (const AbstractGroupableItem * child, item->parentGroup()->members()) {
        if (child == item) {
            break;
        }

        ++row;
    }

    return row;
}

void TasksModelPrivate::itemAboutToBeAdded(AbstractGroupableItem *item, int index)
{
    TaskGroup *parent = item->parentGroup();
    QModelIndex parentIdx;
    if (parent->parentGroup()) {
        parentIdx = q->createIndex(indexOf(parent), 0, parent);
    }

    QObject::connect(item, SIGNAL(changed(::TaskManager::TaskChanges)),
                    q, SLOT(itemChanged(::TaskManager::TaskChanges)),
                    Qt::UniqueConnection);

    q->beginInsertRows(parentIdx, index, index);

    if (item->parentGroup() == rootGroup) {
        QMetaObject::invokeMethod(q, "countChanged", Qt::QueuedConnection);
    }

    if (item->itemType() == LauncherItemType) {
        QMetaObject::invokeMethod(q, "launcherCountChanged", Qt::QueuedConnection);
    }
}

void TasksModelPrivate::itemAdded(AbstractGroupableItem *item)
{
    Q_UNUSED(item);
    q->endInsertRows();
}

void TasksModelPrivate::itemAboutToBeRemoved(AbstractGroupableItem *item)
{
    TaskGroup *parent = static_cast<TaskGroup *>(q->sender());
    QModelIndex parentIdx;
    if (parent && parent->parentGroup()) {
        parentIdx = q->createIndex(indexOf(parent), 0, parent);
    }

    const int index = indexOf(item);
    q->beginRemoveRows(parentIdx, index, index);

    if (item->parentGroup() == rootGroup) {
        QMetaObject::invokeMethod(q, "countChanged", Qt::QueuedConnection);
    }

    if (item->itemType() == LauncherItemType) {
        QMetaObject::invokeMethod(q, "launcherCountChanged", Qt::QueuedConnection);
    }
}

void TasksModelPrivate::itemRemoved(AbstractGroupableItem *item)
{
    Q_UNUSED(item);
    q->endRemoveRows();
}

void TasksModelPrivate::itemAboutToMove(AbstractGroupableItem *item, int currentIndex, int newIndex)
{
    QModelIndex parentIdx;

    TaskGroup *parent = item->parentGroup();
    if (parent && parent->parentGroup()) {
        parentIdx = q->createIndex(indexOf(parent), 0, parent);
    }

    if (newIndex > currentIndex) {
        newIndex++;
    }

    q->beginMoveRows(parentIdx, currentIndex, currentIndex, parentIdx, newIndex);
}

void TasksModelPrivate::itemMoved(AbstractGroupableItem *item)
{
    Q_UNUSED(item)
    q->endMoveRows();
}

void TasksModelPrivate::itemChanged(::TaskManager::TaskChanges changes)
{
    Q_UNUSED(changes)
    AbstractGroupableItem *item = static_cast<AbstractGroupableItem *>(q->sender());
    const int index = indexOf(item);
    QModelIndex idx = q->createIndex(index, 0, item);
    emit q->dataChanged(idx, idx);
}

}

#include "moc_tasksmodel.cpp"

