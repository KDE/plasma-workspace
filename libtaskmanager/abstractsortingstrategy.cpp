/*****************************************************************

Copyright 2008 Christian Mollekopf <chrigi_1@hotmail.com>

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

******************************************************************/

#include "abstractsortingstrategy.h"

#include "taskitem.h"
#include "taskgroup.h"
#include "taskmanager.h"
#include "abstractgroupableitem.h"

#include <QtAlgorithms>
#include <QList>

#include <QDebug>

namespace TaskManager
{

class AbstractSortingStrategy::Private
{
public:
    Private()
        : type(GroupManager::NoSorting) {
    }

    QList<TaskGroup*> managedGroups;
    GroupManager::TaskSortingStrategy type;
};


AbstractSortingStrategy::AbstractSortingStrategy(QObject *parent)
    : QObject(parent),
      d(new Private)
{

}

AbstractSortingStrategy::~AbstractSortingStrategy()
{
    delete d;
}

GroupManager::TaskSortingStrategy AbstractSortingStrategy::type() const
{
    return d->type;
}

void AbstractSortingStrategy::setType(GroupManager::TaskSortingStrategy type)
{
    d->type = type;
}

void AbstractSortingStrategy::handleGroup(TaskGroup *group)
{
    //kDebug();
    if (d->managedGroups.contains(group) || !group) {
        return;
    }

    d->managedGroups.append(group);
    disconnect(group, 0, this, 0); //To avoid duplicate connections
    connect(group, SIGNAL(destroyed()), this, SLOT(removeGroup())); //FIXME necessary?
    ItemList sortedList = group->members();
    sortItems(sortedList); //the sorting doesn't work with totally unsorted lists, therefore we sort it in the correct order the first time

    foreach (AbstractGroupableItem * item, sortedList) {
        handleItem(item);
    }
}

void AbstractSortingStrategy::removeGroup()
{
    TaskGroup *group = dynamic_cast<TaskGroup*>(sender());

    if (!group) {
        return;
    }

    d->managedGroups.removeAll(group);
}

void AbstractSortingStrategy::handleItem(AbstractGroupableItem *item)
{
    //kDebug() << item->name();
    if (item->itemType() == GroupItemType) {
        handleGroup(qobject_cast<TaskGroup*>(item));
    } else if (item->itemType() == TaskItemType && !(qobject_cast<TaskItem*>(item))->task()) { //ignore startup tasks
        connect(item, SIGNAL(gotTaskPointer()), this, SLOT(check())); //sort the task as soon as it is a real one
        return;
    }

    check(item);
}

void AbstractSortingStrategy::check(AbstractGroupableItem *itemToCheck)
{
    AbstractGroupableItem *item;
    if (!itemToCheck) {
        item = dynamic_cast<AbstractGroupableItem *>(sender());
    } else {
        item = itemToCheck;
    }

    if (!item) {
        qWarning() << "invalid item" << itemToCheck;
        return;
    }
    //kDebug() << item->name();

    if (item->itemType() == TaskItemType) {
        if (!(qobject_cast<TaskItem*>(item))->task()) { //ignore startup tasks
            return;
        }
    }

    if (!item->parentGroup()) {
        //kDebug() << "No parent group";
        return;
    }

    ItemList sortedList = item->parentGroup()->members();
    sortItems(sortedList);

    int oldIndex = item->parentGroup()->members().indexOf(item);
    int newIndex = sortedList.indexOf(item);
    if (oldIndex != newIndex) {
        item->parentGroup()->moveItem(oldIndex, newIndex);
    }
}

void AbstractSortingStrategy::sortItems(ItemList &items)
{
    Q_UNUSED(items)
}

bool AbstractSortingStrategy::manualSortingRequest(AbstractGroupableItem *item, int newIndex)
{
    Q_UNUSED(item);
    Q_UNUSED(newIndex);
    return false;
}

bool AbstractSortingStrategy::moveItem(AbstractGroupableItem *item, int newIndex)
{
    //kDebug() << "move to " << newIndex;
    if (!item->parentGroup()) {
        qWarning() << "error: no parentgroup but the item was asked to move";
        return false;
    }

    const ItemList list = item->parentGroup()->members();
    if ((newIndex < 0) || (newIndex >= list.size())) {
        newIndex = list.size();
    }

    int oldIndex = list.indexOf(item);
    if (newIndex > oldIndex) {
        newIndex--; //the index has to be adjusted if we move the item from right to left because the item on the left is removed first
    }

    if (oldIndex != newIndex) {
        return item->parentGroup()->moveItem(oldIndex, newIndex);
    }

    return false;
}

} //namespace

#include "abstractsortingstrategy.moc"
