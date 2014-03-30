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

#ifndef ACTIVITYSORTINGSTRATEGY_H
#define ACTIVITYSORTINGSTRATEGY_H

#include <abstractsortingstrategy.h>

#include <QtCore/QHash>

namespace TaskManager {

/** 
 * Sorts the tasks by activity.
 * The activities with more tasks will be before than the ones with less tasks.
 * If one task is in various activities, the one with more tasks will be taken into account.
 */
class ActivitySortingStrategy: public AbstractSortingStrategy
{
    Q_OBJECT

public:
    ActivitySortingStrategy(QObject *parent);
    virtual ~ActivitySortingStrategy();

    void sortItems(ItemList& items);

protected Q_SLOTS:
    void handleItem(AbstractGroupableItem *item);
    void checkChanges(::TaskManager::TaskChanges changes, ::TaskManager::AbstractGroupableItem *item = 0);

private:
    /**
     * Checks if the order of the activities stored in the instance is still valid and if not, it stores the
     * correct order. */
    bool checkActivitiesOrder(ItemList& items);
    void addActivitiesToActivityCount(ItemList& items, QHash<QString, int>& activityCount);
    static bool lessThanActivityData(QPair<QString, int> &d1, QPair<QString, int> &d2);
    class Comparator;
    QStringList m_activitiesOrder;
};

}

#endif
