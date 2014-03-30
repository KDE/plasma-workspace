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

#ifndef MANUALGROUPINGSTRATEGY_H
#define MANUALGROUPINGSTRATEGY_H

#include "abstractgroupingstrategy.h"
#include "taskgroup.h"
//#include "taskmanager.h"

#include <QObject>

namespace TaskManager
{

class ManualGroupingStrategy;
class GroupManager;
/**
 * Allows to manually group tasks
 */
class ManualGroupingStrategy: public AbstractGroupingStrategy
{
    Q_OBJECT
public:
    ManualGroupingStrategy(GroupManager *groupManager);
    ~ManualGroupingStrategy();

    /** looks up if this item has been grouped before and groups it accordingly.
    *otherwise the item goes to the rootGroup
    */
    void handleItem(AbstractGroupableItem *);
    /** Should be called if the user wants to manually add an item to a group */
    //bool addItemToGroup(AbstractGroupableItem*, TaskGroup*);
    /** Should be called if the user wants to group items manually */
    bool groupItems(ItemList items);

    /** Returns list of actions that a task can do in this groupingStrategy
    *  fore example: remove this Task from this group
    */
    QList<QAction*> strategyActions(QObject *parent, AbstractGroupableItem *item);

    EditableGroupProperties editableGroupProperties();

protected Q_SLOTS:
    /** Checks if the group is still necessary, removes group if empty*/
    //void checkGroup();

private:
    bool manualGrouping(TaskItem* taskItem, TaskGroup* groupItem);

private Q_SLOTS:

    /** Actions which the strategy offers*/
    /** sender item leaves group*/
    void leaveGroup();
    /** Removes all items from the sender group and adds to the parent Group*/
    void removeGroup();

private:
    class Private;
    Private * const d;
};


}



#endif
