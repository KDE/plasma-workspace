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

#ifndef ABSTRACTSORTINGSTRATEGY_H
#define ABSTRACTSORTINGSTRATEGY_H

#include <QtCore/QObject>

#include <abstractgroupableitem.h>
#include <groupmanager.h>
#include <taskmanager_export.h>

namespace TaskManager
{

/**
 * Base class for strategies which can be used to
 * automatically sort tasks.
 */
class TASKMANAGER_EXPORT AbstractSortingStrategy : public QObject
{
    Q_OBJECT
public:
    AbstractSortingStrategy(QObject *parent);
    virtual ~AbstractSortingStrategy();

    /** Returns the strategy type */
    GroupManager::TaskSortingStrategy type() const;

    /** Adds group under control of sorting strategy. all added subgroups are automatically added to this sortingStrategy*/
    void handleGroup(TaskGroup *);

    /** Moves Item to new index*/
    bool moveItem(AbstractGroupableItem *, int);
    /**Reimplement this to support manual sorting*/
    virtual bool manualSortingRequest(AbstractGroupableItem *item, int newIndex);

public Q_SLOTS:
    /** Handles a new item, is typically called after an item was added to a handled group*/
    virtual void handleItem(AbstractGroupableItem *);
    /** Checks if the order has to be updated. Must be connected to a AbstractGroupableItem* */
    void check(AbstractGroupableItem *item = 0);

protected Q_SLOTS:
    void removeGroup(); //FIXME necessary?

protected:
    void setType(GroupManager::TaskSortingStrategy strategy);

private:
    /**
     * Sorts list of items according to startegy.
     * Has to be reimplemented by every SortingStrategy.
     *
     * @param items the items that are to be sorted; the list is passed
     *        in by value and should be in the proprer sorting order when
     *        the method returns.
     */
    virtual void sortItems(ItemList &ites);

    class Private;
    Private * const d;
};

typedef QHash <int, WindowList> itemHashTable;
typedef QHash <int, itemHashTable*> desktopHashTable;

} // TaskManager namespace
#endif
