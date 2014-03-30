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

#include "manualsortingstrategy.h"

#include <QMap>

namespace TaskManager
{

ManualSortingStrategy::ManualSortingStrategy(GroupManager *parent)
    : AbstractSortingStrategy(parent)
{
    setType(GroupManager::ManualSorting);
}

ManualSortingStrategy::~ManualSortingStrategy()
{
}

bool ManualSortingStrategy::manualSortingRequest(AbstractGroupableItem *item, int newIndex)
{
    GroupManager *gm = qobject_cast<GroupManager *>(parent());

    if (!gm) {
        return false;
    }

    if (gm->separateLaunchers()) {
        return moveItem(item, newIndex);
    }

    int oldIndex = gm->launcherIndex(item->launcherUrl());

    if (LauncherItemType == item->itemType() || -1 != oldIndex) {
        bool moveRight = oldIndex > -1 && newIndex > oldIndex;

        if (newIndex >= 0 && newIndex - (moveRight ? 1 : 0) < gm->launcherCount()) {
            if (moveItem(item, newIndex)) {
                gm->moveLauncher(item->launcherUrl(), (newIndex > oldIndex ? --newIndex : newIndex));
                return true;
            }
        }
    } else if (newIndex >= gm->launcherCount()) {
        return moveItem(item, newIndex);
    }

    return false;
}

void ManualSortingStrategy::sortItems(ItemList &items)
{
    GroupManager *gm = qobject_cast<GroupManager *>(parent());

    if (!gm || gm->separateLaunchers()) {
        return;
    }

    //kDebug();
    QMap<int, AbstractGroupableItem*> launcherMap;
    QList<QString> order;
    QMap<QString, AbstractGroupableItem*> map;

    foreach (AbstractGroupableItem * groupable, items) {
        if (!groupable) {
            continue;
        }
        int index = gm ? gm->launcherIndex(groupable->launcherUrl()) : -1;
        if (index < 0) {
            QString name(groupable->name().toLower());
            map.insertMulti(name, groupable);
            if (!order.contains(name)) {
                order.append(name);
            }
        } else {
            launcherMap.insertMulti(index, groupable);
        }
    }

    items.clear();
    items << launcherMap.values();

    foreach (QString n, order) {
        items << map.values(n);
    }
}

} //namespace
#include "manualsortingstrategy.moc"

