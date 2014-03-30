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

#include "manualgroupingstrategy.h"

#include <QAction>
#include <QWeakPointer>

#include <KLocalizedString>

#include "abstractgroupingstrategy.h"
#include "groupmanager.h"
#include "taskmanager.h"


namespace TaskManager
{

class ManualGroupingStrategy::Private
{
public:
    Private()
        : editableGroupProperties(AbstractGroupingStrategy::All),
          tempItem(0) {
    }

    AbstractGroupingStrategy::EditableGroupProperties editableGroupProperties;
    AbstractGroupableItem *tempItem;
    QWeakPointer<TaskGroup> tempGroup;
};



ManualGroupingStrategy::ManualGroupingStrategy(GroupManager *groupManager)
    : AbstractGroupingStrategy(groupManager),
      d(new Private)
{
    setType(GroupManager::ManualGrouping);
}

ManualGroupingStrategy::~ManualGroupingStrategy()
{
    delete d;
}

AbstractGroupingStrategy::EditableGroupProperties ManualGroupingStrategy::editableGroupProperties()
{
    return d->editableGroupProperties;
}

QList<QAction*> ManualGroupingStrategy::strategyActions(QObject *parent, AbstractGroupableItem *item)
{
    QList<QAction*> actionList;

    if (item->isGrouped()) {
        QAction *a = new QAction(i18n("Leave Group"), parent);
        connect(a, SIGNAL(triggered()), this, SLOT(leaveGroup()));
        actionList.append(a);
        d->tempItem = item;
    }

    if (item->itemType() == GroupItemType) {
        QAction *a = new QAction(i18n("Remove Group"), parent);
        connect(a, SIGNAL(triggered()), this, SLOT(removeGroup()));
        actionList.append(a);
        d->tempGroup = dynamic_cast<TaskGroup*>(item);
    }

    return actionList;
}

void ManualGroupingStrategy::leaveGroup()
{
    Q_ASSERT(d->tempItem);
    if (d->tempItem->isGrouped()) {
        d->tempItem->parentGroup()->parentGroup()->add(d->tempItem);
    }
    d->tempItem = 0;
}

void ManualGroupingStrategy::removeGroup()
{
    TaskGroup *tempGroup = d->tempGroup.data();
    if (!tempGroup) {
        return;
    }

    TaskGroup *parentGroup = tempGroup->parentGroup(); //tempGroup is invalid before last item has been moved to the parentGroup
    if (parentGroup) {
        foreach (AbstractGroupableItem * item, tempGroup->members()) {
            parentGroup->add(item);
        }
        //Group gets automatically closed on empty signal
    }

    d->tempGroup.clear();
}

void ManualGroupingStrategy::handleItem(AbstractGroupableItem *item)
{
    if (!rootGroup()) {
        return;
    }
    rootGroup()->add(item);
}



}//namespace

#include "manualgroupingstrategy.moc"

