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

#ifndef ABSTRACTGROUPABLEITEM_H
#define ABSTRACTGROUPABLEITEM_H

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtGui/QIcon>

#include "taskmanager.h"
#include "taskmanager_export.h"

namespace TaskManager
{

class TaskGroup;
class AbstractGroupableItem;

enum ItemType {
    GroupItemType,
    LauncherItemType,
    TaskItemType
};

typedef TaskGroup* GroupPtr;

typedef QList<AbstractGroupableItem *> ItemList;
typedef QList<GroupPtr> GroupList;

/**
 * Abstract Class for an Item that is groupable
 * So groups can handle tasks and subgroups the same way
 */
class TASKMANAGER_EXPORT AbstractGroupableItem : public QObject
{
    Q_OBJECT
public:
    AbstractGroupableItem(QObject *parent);
    virtual ~AbstractGroupableItem();

    virtual QIcon icon() const;
    virtual QString name() const;
    virtual QString genericName() const;

    virtual WindowList winIds() const;

    /**
     * @return unique identifier of this item
     */
    int id() const;

    /**
    *   Returns the parent group of this item
    */
    GroupPtr parentGroup() const;

    /**
    *   Not only member of rootGroup
    */
    bool isGrouped() const;

    bool isGroupMember(const GroupPtr group) const;
    virtual ItemType itemType() const = 0;
    /**
    * @deprecated: use itemType() instead
    **/
    TASKMANAGER_DEPRECATED virtual bool isGroupItem() const = 0;

    virtual bool isStartupItem() const;

    virtual bool isOnCurrentDesktop() const = 0;
    virtual bool isOnAllDesktops() const = 0;
    virtual int desktop() const = 0;
    virtual bool isShaded() const = 0;
    virtual bool isMaximized() const = 0;
    virtual bool isMinimized() const = 0;
    virtual bool isFullScreen() const = 0;
    virtual bool isKeptBelowOthers() const = 0;
    virtual bool isAlwaysOnTop() const = 0;
    virtual bool isActionSupported(NET::Action) const = 0;
    virtual bool isActive() const = 0;
    virtual bool demandsAttention() const = 0;
    virtual void addMimeData(QMimeData *) const = 0;
    virtual QUrl launcherUrl() const = 0;

public Q_SLOTS:
    /** Functions that both, Tasks and Groups have */
    virtual void toDesktop(int) = 0;

    virtual void setShaded(bool) = 0;
    virtual void toggleShaded() = 0;

    virtual void setMaximized(bool) = 0;
    virtual void toggleMaximized() = 0;

    virtual void setMinimized(bool) = 0;
    virtual void toggleMinimized() = 0;

    virtual void setFullScreen(bool) = 0;
    virtual void toggleFullScreen() = 0;

    virtual void setKeptBelowOthers(bool) = 0;
    virtual void toggleKeptBelowOthers() = 0;

    virtual void setAlwaysOnTop(bool) = 0;
    virtual void toggleAlwaysOnTop() = 0;

    virtual void close() = 0;

    void setParentGroup(const GroupPtr group);
    /*void removedFromGroup();
    void addedToGroup(const GroupPtr group);*/

Q_SIGNALS:
    void changed(::TaskManager::TaskChanges changes);
    void destroyed(AbstractGroupableItem *);

private:
    class Private;
    Private * const d;
};

} // TaskManager namespace

#endif
