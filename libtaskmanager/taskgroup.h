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

 ***************************************************************************/

#ifndef TASKGROUP_H
#define TASKGROUP_H

#include <QtGui/QIcon>

#include "abstractgroupableitem.h"
#include "taskmanager_export.h"
#include "launcheritem.h"

namespace TaskManager
{
class GroupManager;

/**
 * TaskGroup, a container for tasks and subgroups
 */
class TASKMANAGER_EXPORT TaskGroup : public AbstractGroupableItem
{
    Q_OBJECT
public:
    TaskGroup(GroupManager *parent, const QString& name);
    TaskGroup(GroupManager *parent);
    ~TaskGroup();

    GroupManager *manager() const;
    ItemList members() const;
    WindowList winIds() const;
    WindowList directMemberwinIds() const;

    AbstractGroupableItem *getMemberByWId(WId id);
    AbstractGroupableItem *getMemberById(int id);
    //including subgroups
    int totalSize();

    QIcon icon() const;
    void setIcon(const QIcon &icon);

    QString name() const;
    void setName(const QString &newName);

    ItemType itemType() const;
    /**
    * @deprecated: use itemType() instead
    **/
    TASKMANAGER_DEPRECATED bool isGroupItem() const;
    bool isRootGroup() const;

    /** only true if item is in this group */
    bool hasDirectMember(AbstractGroupableItem * item) const;
    /** only true if item is in this or any sub group */
    bool hasMember(AbstractGroupableItem * item) const;
    /** Returns Direct Member group if the passed item is in a subgroup */
    AbstractGroupableItem * directMember(AbstractGroupableItem *) const;

    int desktop() const;
    bool isShaded() const;
    bool isMaximized() const;
    bool isMinimized() const;
    bool isFullScreen() const;
    bool isKeptBelowOthers() const;
    bool isAlwaysOnTop() const;
    bool isActionSupported(NET::Action) const;
    /** returns true if at least one member is active */
    bool isActive() const;
    /** returns true if at least one member is demands attention */
    bool demandsAttention() const;
    bool isOnAllDesktops() const;
    bool isOnCurrentDesktop() const;
    void addMimeData(QMimeData *mimeData) const;
    QUrl launcherUrl() const;

    /**
     * Sorting strategies may use this to move items around
     * @param oldIndex the index the item to be moved is currently at
     * @param newIndex the index the item will be moved to
     */
    bool moveItem(int oldIndex, int newIndex);

public Q_SLOTS:
    /** the following are functions which perform the corresponding actions on all member tasks */
    void toDesktop(int);

    void setShaded(bool);
    void toggleShaded();

    void setMaximized(bool);
    void toggleMaximized();

    void setMinimized(bool);
    void toggleMinimized();

    void setFullScreen(bool);
    void toggleFullScreen();

    void setKeptBelowOthers(bool);
    void toggleKeptBelowOthers();

    void setAlwaysOnTop(bool);
    void toggleAlwaysOnTop();

    /** close all members of this group */
    void close();

    /** add item to group */
    void add(AbstractGroupableItem *item, int insertIndex = -1);

    /** remove item from group */
    void remove(AbstractGroupableItem *);

    /** remove all items from group */
    void clear();

Q_SIGNALS:
    /** inform visualization about wat is added and removed */
    void itemAboutToBeAdded(AbstractGroupableItem *item, int index);
    void itemAdded(AbstractGroupableItem *item);
    void itemAboutToBeRemoved(AbstractGroupableItem *item);
    void itemRemoved(AbstractGroupableItem *item);
    void groupEditRequest();
    /** inform visualization about position change */
    void itemAboutToMove(AbstractGroupableItem *item, int currentIndex, int newIndex);
    void itemPositionChanged(AbstractGroupableItem *item);
    /** The group changed the desktop, is emitted in the toDesktop function */
    void movedToDesktop(int newDesk);
    void checkIcon(TaskGroup *group);

private:
    Q_PRIVATE_SLOT(d, void itemDestroyed(AbstractGroupableItem *item))
    Q_PRIVATE_SLOT(d, void itemChanged(::TaskManager::TaskChanges changes))
    Q_PRIVATE_SLOT(d, void signalRemovals())

    class Private;
    Private * const d;
};


} // TaskManager namespace

#endif
