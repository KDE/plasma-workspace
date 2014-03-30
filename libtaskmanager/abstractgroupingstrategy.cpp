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

#include "abstractgroupingstrategy.h"

#include <QtCore/QTimer>

#include "task.h"

namespace TaskManager
{

class AbstractGroupingStrategy::Private
{
public:
    Private()
        : type(GroupManager::NoGrouping) {
    }

    GroupManager *groupManager;
    QStringList usedNames;
    QList<TaskGroup*> createdGroups;
    GroupManager::TaskGroupingStrategy type;
};

AbstractGroupingStrategy::AbstractGroupingStrategy(GroupManager *groupManager)
    : QObject(groupManager),
      d(new Private)
{
    d->groupManager = groupManager;
}

AbstractGroupingStrategy::~AbstractGroupingStrategy()
{
    destroy();
    qDeleteAll(d->createdGroups);
    delete d;
}

void AbstractGroupingStrategy::destroy()
{
    if (!d->groupManager) {
        return;
    }

    // cleanup all created groups
    foreach (TaskGroup * group, d->createdGroups) {
        disconnect(group, 0, this, 0);

        TaskGroup *parentGroup = group->parentGroup();
        if (!parentGroup) {
            parentGroup = d->groupManager->rootGroup();
        }

        foreach (AbstractGroupableItem * item, group->members()) {
            if (item->itemType() != GroupItemType) {
                parentGroup->add(item);
            }
        }

        parentGroup->remove(group);
    }

    foreach (TaskGroup * group, d->createdGroups) {
        emit groupRemoved(group);
    }

    d->groupManager = 0;
    deleteLater();
}

GroupManager::TaskGroupingStrategy AbstractGroupingStrategy::type() const
{
    return d->type;
}

void AbstractGroupingStrategy::setType(GroupManager::TaskGroupingStrategy type)
{
    d->type = type;
}

void AbstractGroupingStrategy::desktopChanged(int newDesktop)
{
    Q_UNUSED(newDesktop)
}

QList<QAction*> AbstractGroupingStrategy::strategyActions(QObject *parent, AbstractGroupableItem *item)
{
    Q_UNUSED(parent)
    Q_UNUSED(item)
    return QList<QAction*>();
}

GroupPtr AbstractGroupingStrategy::rootGroup() const
{
    if (d->groupManager) {
        return d->groupManager->rootGroup();
    }

    return 0;
}

TaskGroup* AbstractGroupingStrategy::createGroup(ItemList items)
{
    GroupPtr oldGroup;
    if (!items.isEmpty() && items.first()->isGrouped()) {
        oldGroup = items.first()->parentGroup();
    } else {
        oldGroup = rootGroup();
    }

    TaskGroup *newGroup = new TaskGroup(d->groupManager);
    ItemList oldGroupMembers = oldGroup->members();
    int index = oldGroupMembers.count();
    d->createdGroups.append(newGroup);
    //kDebug() << "added group" << d->createdGroups.count();
    // NOTE: Queued is vital to make sure groups only get removed after their children, for
    // correct QAbstractItemModel (TasksModel) transaction semantics.
    connect(newGroup, SIGNAL(itemRemoved(AbstractGroupableItem*)), this, SLOT(checkGroup()), Qt::QueuedConnection);
    foreach (AbstractGroupableItem * item, items) {
        int idx = oldGroupMembers.indexOf(item);
        if (idx >= 0 && idx < index) {
            index = idx;
        }
        newGroup->add(item);
    }

    Q_ASSERT(oldGroup);
    // Place new group where first of the moved items was...
    oldGroup->add(newGroup, index);

    return newGroup;
}

void AbstractGroupingStrategy::closeGroup(TaskGroup *group)
{
    Q_ASSERT(group);
    disconnect(group, 0, this, 0);
    d->createdGroups.removeAll(group);
    //kDebug() << "closig group" << group->name() << d->createdGroups.count();
    d->usedNames.removeAll(group->name());
    //d->usedIcons.removeAll(group->icon());//TODO

    TaskGroup *parentGroup = group->parentGroup();
    if (!parentGroup) {
        parentGroup = rootGroup();
    }

    if (parentGroup && d->groupManager) {
        int index = parentGroup->members().indexOf(group);
        foreach (AbstractGroupableItem * item, group->members()) {
            parentGroup->add(item, index);
            //move item to the location where its group was
            if (!d->groupManager) {
                // this means that the above add() caused a change in grouping strategy
                break;
            }
        }
        parentGroup->remove(group);
    }

    emit groupRemoved(group);
    // FIXME: due to a bug in Qt 4.x, the event loop reference count is incorrect
    // when going through x11EventFilter .. :/ so we have to singleShot the deleteLater
    QTimer::singleShot(0, group, SLOT(deleteLater()));
}

void AbstractGroupingStrategy::checkGroup()
{
    TaskGroup *group = qobject_cast<TaskGroup*>(sender());
    if (!group) {
        return;
    }

    if (group->members().size() <= 0) {
        closeGroup(group);
    }
}

bool AbstractGroupingStrategy::manualGroupingRequest(AbstractGroupableItem* taskItem, TaskGroup* groupItem)
{
    if (editableGroupProperties() & Members) {
        groupItem->add(taskItem);
        return true;
    }
    return false;
}

//TODO move to manual strategy?
bool AbstractGroupingStrategy::manualGroupingRequest(ItemList items)
{
    if (editableGroupProperties() & Members) {
        TaskGroup *group = createGroup(items);
        setName(nameSuggestions(group).first(), group);
        setIcon(iconSuggestions(group).first(), group);
        return true;
    }
    return false;
}

bool AbstractGroupingStrategy::setName(const QString &name, TaskGroup *group)
{
    d->usedNames.removeAll(group->name());
    if ((editableGroupProperties() & Name) && (!d->usedNames.contains(name))) {
        d->usedNames.append(name);
        group->setName(name);
        return true;
    }
    return false;
}

//Returns 6 free names
QList<QString> AbstractGroupingStrategy::nameSuggestions(TaskGroup *)
{
    QList<QString> nameList;
    int i = 1;

    while (nameList.count() < 6) {
        if (!d->usedNames.contains("Group" + QString::number(i))) {
            nameList.append("Group" + QString::number(i));
        }
        i++;
    }

    if (nameList.isEmpty()) {
        nameList.append("default");
    }

    return nameList;
}

bool AbstractGroupingStrategy::setIcon(const QIcon &icon, TaskGroup *group)
{
    if (editableGroupProperties() & Icon) {
        group->setIcon(icon);
        return true;
    }

    return false;
}

QList <QIcon> AbstractGroupingStrategy::iconSuggestions(TaskGroup *)
{
    QList <QIcon> iconList;
    iconList.append(QIcon::fromTheme("xorg"));
    return iconList;
}

}//namespace

#include "abstractgroupingstrategy.moc"

