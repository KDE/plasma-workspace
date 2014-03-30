/***************************************************************** Copyright 2008 Christian Mollekopf <chrigi_1@hotmail.com>
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

#include "programgroupingstrategy.h"

#include <QAction>
#include <QWeakPointer>

#include <KLocalizedString>
#include <KConfig>
#include <KConfigGroup>

#include "abstractgroupingstrategy.h"
#include "groupmanager.h"

namespace TaskManager
{

class ProgramGroupingStrategy::Private
{
public:
    Private()
        : editableGroupProperties(AbstractGroupingStrategy::None) {
    }

    AbstractGroupingStrategy::EditableGroupProperties editableGroupProperties;
    QWeakPointer<AbstractGroupableItem> tempItem;
    QStringList blackList; //Programs in this list should not be grouped
};


ProgramGroupingStrategy::ProgramGroupingStrategy(GroupManager *groupManager)
    : AbstractGroupingStrategy(groupManager),
      d(new Private)
{
    setType(GroupManager::ProgramGrouping);

    KConfig groupBlacklist("taskbargroupblacklistrc", KConfig::NoGlobals);
    KConfigGroup blackGroup(&groupBlacklist, "Blacklist");
    d->blackList = blackGroup.readEntry("Applications", QStringList());
}

ProgramGroupingStrategy::~ProgramGroupingStrategy()
{
    delete d;
}

QList<QAction*> ProgramGroupingStrategy::strategyActions(QObject *parent, AbstractGroupableItem *item)
{
    QList<QAction*> actionList;

    GroupManager *gm = qobject_cast<GroupManager *>(AbstractGroupingStrategy::parent());

    if (!gm || !gm->forceGrouping()) {
        QAction *a = new QAction(parent);
        QString name = className(item);
        if (d->blackList.contains(name)) {
            a->setText(i18n("Allow this program to be grouped"));
        } else {
            a->setText(i18n("Do not allow this program to be grouped"));
        }
        connect(a, SIGNAL(triggered()), this, SLOT(toggleGrouping()), Qt::QueuedConnection);

        actionList.append(a);
        d->tempItem = item;
    }
    return actionList;
}

QString ProgramGroupingStrategy::className(AbstractGroupableItem *item)
{
    QString name;
    if (item->itemType() == GroupItemType) { //maybe add the condition that the subgroup was created by programGrouping
        TaskGroup *group = qobject_cast<TaskGroup*>(item);
        TaskItem *task = qobject_cast<TaskItem*>(group->members().first()); //There are only TaskItems in programGrouping groups
        return task->task()->classClass();
    }

    return (qobject_cast<TaskItem*>(item))->task()->classClass();
}


void ProgramGroupingStrategy::toggleGrouping()
{
    AbstractGroupableItem *tempItem = d->tempItem.data();
    if (!tempItem) {
        return;
    }

    QString name = className(tempItem);

    if (d->blackList.contains(name)) {
        d->blackList.removeAll(name);
        if (tempItem->itemType() == GroupItemType) {
            foreach (AbstractGroupableItem * item, (qobject_cast<TaskGroup*>(tempItem))->members()) {
                handleItem(item);
            }
        } else {
            handleItem(tempItem);
        }
    } else {
        d->blackList.append(name);
        GroupPtr root = rootGroup();
        if (tempItem->itemType() == GroupItemType) {
            closeGroup(qobject_cast<TaskGroup*>(tempItem));
        } else if (root) {
            root->add(tempItem);
        }

        if (root) {
            foreach (AbstractGroupableItem * item, root->members()) {
                if (item->itemType() == GroupItemType) {
                    untoggleGroupingOn(static_cast<TaskGroup*>(item), name);
                }
            }
        }
    }

    d->tempItem.clear();

    // Save immediately. Much better than saving in the destructor,
    // since this class is deleted at every change of virtual desktop (!)
    // So when doing it from the destructor we were constantly sync'ing
    // (and triggering KDirWatch) even when the blacklist hadn't changed at all...
    KConfig groupBlacklist("taskbargroupblacklistrc", KConfig::NoGlobals);
    KConfigGroup blackGroup(&groupBlacklist, "Blacklist");
    blackGroup.writeEntry("Applications", d->blackList);
    blackGroup.config()->sync();
}

void ProgramGroupingStrategy::untoggleGroupingOn(TaskGroup *group, const QString &name)
{
    if (!group->parentGroup()) {
        return;
    }

    foreach (AbstractGroupableItem * item, group->members()) {
        if (item->itemType() == GroupItemType) {
            untoggleGroupingOn(static_cast<TaskGroup*>(item), name);
        }
    }

    foreach (AbstractGroupableItem * item, group->members()) {
        if (item->itemType() != GroupItemType && name == static_cast<TaskItem *>(item)->task()->classClass())  {
            if (group->parentGroup()) {
                group->parentGroup()->add(item);
            }
        }
    }

    if (group->members().isEmpty()) {
        closeGroup(group);
    }
}

void ProgramGroupingStrategy::handleItem(AbstractGroupableItem *item)
{
    GroupPtr root = rootGroup();

    if (!root) {
        return;
    }

    GroupManager *gm = qobject_cast<GroupManager *>(AbstractGroupingStrategy::parent());

    if (item->itemType() == GroupItemType) {
        //kDebug() << item->name() << "item is groupitem";
        root->add(item);
        return;
    } else if ((!gm || !gm->forceGrouping()) && d->blackList.contains((static_cast<TaskItem*>(item))->task()->classClass())) {
        //kDebug() << item->name() << "item is in blacklist";
        root->add(item);
        return;
    }

    TaskItem *task = dynamic_cast<TaskItem*>(item);
    if (task && !programGrouping(task, root)) {
        //kDebug() << item->name() << "joined rootGroup ";
        root->add(item);
    }
}

bool ProgramGroupingStrategy::programGrouping(TaskItem* taskItem, TaskGroup* groupItem)
{
    //kDebug() << "===== Task:" << taskItem->name() << " <=> Group:" << groupItem->name() << "=====";
    QList<AbstractGroupableItem *> list;
    QString name = taskItem->task()->classClass();

    //search for an existing group
    foreach (AbstractGroupableItem * item, groupItem->members()) {
        if (item->itemType() == GroupItemType) {
            //TODO: maybe add the condition that the subgroup was created by programGrouping?
            if (programGrouping(taskItem, static_cast<TaskGroup*>(item))) {
                //kDebug() << "    joined subGroup";
                return true;
            }
        } else if (item->itemType() == LauncherItemType) {
            continue;
        } else {
            TaskItem *task = static_cast<TaskItem*>(item);
            //kDebug() << "    testing" << (task->task() ? task->task()->classClass() : "No task!") << "==" << name;
            if (task != taskItem && task->task() && task->task()->classClass() == name) { //omit startup tasks
                list.append(item);
            }
        }
    }

    if (!list.isEmpty()) {
        if (groupItem->isRootGroup()) {
            //kDebug() << "    create Group root group";
            QIcon icon = taskItem->icon();
            list.append(taskItem);
            TaskGroup* group = createGroup(list);
            group->setName(name);
            group->setIcon(icon);
            connect(group, SIGNAL(checkIcon(TaskGroup*)), this, SLOT(updateIcon(TaskGroup*)));
        } else {
            //kDebug() << "    joined this Group";
            groupItem->add(taskItem);
        }

        return true;
    }

    //kDebug() << "    failed";
    return false;
}

void ProgramGroupingStrategy::checkGroup()
{
    TaskGroup *group = qobject_cast<TaskGroup*>(sender());
    if (group) {
        if (group->members().size() < 2) {
            closeGroup(group);
        } else {
            updateIcon(group);
        }
    }
}

void ProgramGroupingStrategy::updateIcon(TaskGroup *group)
{
    foreach (AbstractGroupableItem * taskItem, group->members()) {
        if (!taskItem->icon().isNull()) {
            QIcon icon = taskItem->icon();
            group->setIcon(icon);
            break;
        }
    }
}

}//namespace

#include "programgroupingstrategy.moc"

