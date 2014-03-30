/***************************************************************************
 *   Copyright (C) 2013 OPENTIA Group http://opentia.com                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#include <strategies/activitysortingstrategy.h>
#include <taskitem.h>
#include <taskgroup.h>

namespace TaskManager
{

ActivitySortingStrategy::ActivitySortingStrategy(QObject *parent)
    :AbstractSortingStrategy(parent)
{
    setType(GroupManager::ActivitySorting);
}

class ActivitySortingStrategy::Comparator {
public:
    Comparator(QStringList *activitiesOrder) {
        m_activitiesOrder = activitiesOrder;
    }

    bool operator()(const AbstractGroupableItem *i1, const AbstractGroupableItem *i2) {
        if (!m_priorityCache.contains(i1->id())) {
            addToCache(i1);
        }

        if (!m_priorityCache.contains(i2->id())) {
            addToCache(i2);
        }

        QList<int>::const_iterator it1 = m_priorityCache[i1->id()].constBegin();
        QList<int>::const_iterator it2 = m_priorityCache[i2->id()].constBegin();

        QList<int>::const_iterator i1end = m_priorityCache[i1->id()].constEnd();
        QList<int>::const_iterator i2end = m_priorityCache[i2->id()].constEnd();

        while (it1 != i1end) {
            if (it2 == i2end) {
                return true;
            } else if (*it1 == *it2) {
                it1++;
                it2++;
            } else {
                return *it1 < *it2;
            }
        }
        return false;
    }

private:
    QList<int> getActivitiesIndexes(const AbstractGroupableItem *item) {
        QList<int> cacheEntry;

        const TaskItem *taskItem = qobject_cast<const TaskItem*>(item);
        if (taskItem) {
            Q_FOREACH(QString activity, taskItem->activities()) {
                cacheEntry << m_activitiesOrder->indexOf(activity);
            }
        }

        const TaskGroup *taskGroup = qobject_cast<const TaskGroup*>(item);
        if (taskGroup) {
            Q_FOREACH(AbstractGroupableItem *member, taskGroup->members()) {
                QList<int> memberIndexes = getActivitiesIndexes(member);
                Q_FOREACH(int i, memberIndexes) {
                    if (!cacheEntry.contains(i)) {
                        cacheEntry << i;
                    }
                }
            }
        }

        return cacheEntry;
    }

    void addToCache(const AbstractGroupableItem *item) {
        QList<int> cacheEntry = getActivitiesIndexes(item);
        qSort(cacheEntry.begin(), cacheEntry.end());
        m_priorityCache[item->id()] = cacheEntry;
    }

    const QStringList *m_activitiesOrder;
    QHash<int, QList<int> > m_priorityCache;
};

ActivitySortingStrategy::~ActivitySortingStrategy()
{
}

void ActivitySortingStrategy::sortItems(ItemList& items)
{
    if (m_activitiesOrder.isEmpty()) {
        checkActivitiesOrder(items);
    }

    Comparator comparator(&m_activitiesOrder);

    qStableSort(items.begin(), items.end(), comparator);
}

void ActivitySortingStrategy::handleItem(AbstractGroupableItem* item)
{
    connect(item, SIGNAL(changed(::TaskManager::TaskChanges)), this, SLOT(checkChanges(::TaskManager::TaskChanges)));
    AbstractSortingStrategy::handleItem(item);
    checkChanges(ActivitiesChanged, item);
}

bool ActivitySortingStrategy::lessThanActivityData(QPair< QString, int >& d1, QPair< QString, int >& d2)
{
    if (d1.second == d2.second) {
        return d1.first < d2.first;
    }

    return d1.second > d2.second;
}

void ActivitySortingStrategy::addActivitiesToActivityCount(ItemList& items, QHash<QString, int>& activityCount)
{
    Q_FOREACH(AbstractGroupableItem *item, items) {
        const TaskItem *taskItem = qobject_cast< const TaskItem* >(item);
        if (taskItem){
            Q_FOREACH(QString activity, taskItem->activities()) {
                if (activityCount.contains(activity)) {
                    activityCount[activity]++;
                } else {
                    activityCount[activity] = 1;
                }
            }
        }

        const TaskGroup *taskGroup = qobject_cast< const TaskGroup* >(item);
        if (taskGroup) {
            ItemList subItems = taskGroup->members();
            addActivitiesToActivityCount(subItems, activityCount);
        }
    }
}

bool ActivitySortingStrategy::checkActivitiesOrder(ItemList& items)
{
    //Number of tasks by activity
    QHash<QString, int> activityCount;

    addActivitiesToActivityCount(items, activityCount);

    QList<QPair<QString, int> > activityData;
    Q_FOREACH(QString activity, activityCount.keys()) {
        activityData << QPair<QString, int>(activity, activityCount[activity]);
    }
    qSort(activityData.begin(), activityData.end(), lessThanActivityData);


    QStringList newActivitiesOrder;
    QList<QPair<QString, int> >::const_iterator it;
    for (it = activityData.constBegin(); it != activityData.constEnd(); it++) {
        newActivitiesOrder << it->first;
    }

    if (newActivitiesOrder != m_activitiesOrder) {
        m_activitiesOrder = newActivitiesOrder;
        return true;
    }
    return false;
}

void ActivitySortingStrategy::checkChanges(TaskChanges changes, AbstractGroupableItem *item)
{
    if (!item) {
        item = qobject_cast< AbstractGroupableItem* >(sender());
    }

    if (!item) {
        return;
    }

    if (changes & ActivitiesChanged) {
        if (!item->parentGroup()) {
            check(item);
        } else {
            TaskGroup* base = item->parentGroup();
            while (base->parentGroup()) {
                base = base->parentGroup();
            }

            ItemList items = base->members();

            if (checkActivitiesOrder(items)) {
                //If the order of the activities has changed, all elements
                //should be put in order again
                sortItems(items);

                int i = 0;
                Q_FOREACH(AbstractGroupableItem *element, items) {
                    moveItem(element, i);
                    i++;
                }
            } else {
                check(item);
            }
        }
    }
}

}
