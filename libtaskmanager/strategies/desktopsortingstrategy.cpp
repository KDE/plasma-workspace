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

#include "desktopsortingstrategy.h"

#include <QMap>
#include <QString>
#include <QtAlgorithms>
#include <QList>

#include "abstractgroupableitem.h"
#include "groupmanager.h"

namespace TaskManager
{

static QString agiName(const AbstractGroupableItem *i)
{
    if (i->itemType() == TaskItemType && !i->isStartupItem()) {
        return static_cast<const TaskItem *>(i)->taskName().toLower();
    } else {
        return i->name().toLower();
    }
}

DesktopSortingStrategy::DesktopSortingStrategy(QObject *parent)
    : AbstractSortingStrategy(parent)
{
    setType(GroupManager::DesktopSorting);
}

void DesktopSortingStrategy::sortItems(ItemList &items)
{
    GroupManager *gm = qobject_cast<GroupManager *>(parent());
    qStableSort(items.begin(), items.end(), (gm && gm->separateLaunchers()) ?
                                                DesktopSortingStrategy::lessThanSeperateLaunchers :
                                                DesktopSortingStrategy::lessThan);
}

/*
 * Sorting strategy is as follows:
 * For two items being compared
 *   - Startup items will be sorted out to the end of the list and sorted by name there
 *   - If both are not startup tasks first compare items by desktop number,
 *     and then for items which belong to the same desktop sort by their NAME.
 */
bool DesktopSortingStrategy::lessThan(const AbstractGroupableItem *left, const AbstractGroupableItem *right)
{
    const int leftDesktop = left->desktop();
    const int rightDesktop = right->desktop();
    if (leftDesktop == rightDesktop) {
        return agiName(left) < agiName(right);
    }

    return leftDesktop < rightDesktop;
}

bool DesktopSortingStrategy::lessThanSeperateLaunchers(const AbstractGroupableItem *left, const AbstractGroupableItem *right)
{
    if (left->isStartupItem()) {
        if (right->isStartupItem()) {
            return left->name().toLower() < right->name().toLower();
        }
        return false;
    }

    if (right->isStartupItem()) {
        return true;
    }

    if (left->itemType() == LauncherItemType) {
        if (right->itemType() == LauncherItemType) {
            return left->name().toLower() < right->name().toLower();
        }
        return true;
    }

    if (right->itemType() == LauncherItemType) {
        return false;
    }

    return lessThan(left, right);
}

void DesktopSortingStrategy::handleItem(AbstractGroupableItem *item)
{
    disconnect(item, 0, this, 0); //To avoid duplicate connections
    connect(item, SIGNAL(changed(::TaskManager::TaskChanges)), this, SLOT(check()));
    AbstractSortingStrategy::handleItem(item);
}

} //namespace

#include "desktopsortingstrategy.moc"

