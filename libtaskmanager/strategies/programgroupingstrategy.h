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

#ifndef PROGRAMGROUPINGSTRATEGY_H
#define PROGRAMGROUPINGSTRATEGY_H

#include "abstractgroupingstrategy.h"
#include "taskgroup.h"

namespace TaskManager
{

class GroupManager;

/**
 * Groups tasks of the same program
 */
class ProgramGroupingStrategy: public AbstractGroupingStrategy
{
    Q_OBJECT
public:
    ProgramGroupingStrategy(GroupManager *groupManager);
    ~ProgramGroupingStrategy();
    /** Tasks are passed to this function to be grouped by this strategy
    */
    void handleItem(AbstractGroupableItem *);

    /** Returns list of actions that a task can do in this groupingStrategy
    *  fore example: start/stop group tasks of this program
    */
    QList<QAction*> strategyActions(QObject *parent, AbstractGroupableItem *item);

    EditableGroupProperties editableGroupProperties() {
        return None;
    };

protected Q_SLOTS:
    /** Checks if the group is still necessary */
    void checkGroup();
private Q_SLOTS:
    /** The program of the sender() of this function is started or stopped being grouped
    *   by this strategy. This is done by adding the program to d->blackList
    */
    void toggleGrouping();
    void updateIcon(TaskGroup *group);

private:
    QString className(AbstractGroupableItem *item);
    bool programGrouping(TaskItem* taskItem, TaskGroup* groupItem);
    void untoggleGroupingOn(TaskGroup *group, const QString &name);

    class Private;
    Private * const d;
};


}



#endif
