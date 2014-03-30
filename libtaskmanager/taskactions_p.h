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

#ifndef TASKACTIONS_P_H
#define TASKACTIONS_P_H

#include "taskmanager.h"
#include <KOpenWithDialog>
#include <QtCore/QPointer>

namespace TaskManager
{

/** Maximize a window or all windows in a group*/
class MaximizeActionImpl : public QAction
{
    Q_OBJECT
public:
    MaximizeActionImpl(QObject *parent, AbstractGroupableItem *task);
};


/** Minimize a window or all windows in a group*/
class MinimizeActionImpl : public QAction
{
    Q_OBJECT
public:
    MinimizeActionImpl(QObject *parent, AbstractGroupableItem *task);
};

/** Shade a window or all windows in a group*/
class ShadeActionImpl : public QAction
{
    Q_OBJECT
public:
    ShadeActionImpl(QObject *parent, AbstractGroupableItem *task);
};

class ToggleLauncherActionImpl : public QAction
{
    Q_OBJECT
public:
    ToggleLauncherActionImpl(QObject* parent, AbstractGroupableItem* item, GroupManager* strategy);

private Q_SLOTS:
    void toggleLauncher();

private:
    AbstractGroupableItem *m_abstractItem;
    GroupManager *m_groupingStrategy;
    QUrl m_url;
};

class AppSelectorDialog : public KOpenWithDialog
{
    Q_OBJECT

public:
    AppSelectorDialog(AbstractGroupableItem* item, GroupManager* strategy);

private Q_SLOTS:
    void launcherSelected();

private:
    QPointer<AbstractGroupableItem> m_abstractItem;
    QPointer<GroupManager> m_groupingStrategy;
};

class NewInstanceActionImpl : public QAction
{
    Q_OBJECT
public:
    NewInstanceActionImpl(QObject* parent, AbstractGroupableItem* item);

private Q_SLOTS:
    void launchNewInstance();

private:
    AbstractGroupableItem *m_abstractItem;
    QUrl m_url;
};

/** Resize a window or all windows in a group*/
class ResizeActionImpl : public QAction
{
    Q_OBJECT
public:
    ResizeActionImpl(QObject *parent, TaskItem* task);
};

/** Move a window or all windows in a group*/
class MoveActionImpl : public QAction
{
    Q_OBJECT
public:
    MoveActionImpl(QObject *parent, TaskItem* task);
};

/** Shade a window or all windows in a group*/
class CloseActionImpl : public QAction
{
    Q_OBJECT
public:
    CloseActionImpl(QObject *parent, AbstractGroupableItem *task);
};

class AbstractGroupableItemAction : public QAction
{
    Q_OBJECT
public:
    AbstractGroupableItemAction(QObject *parent, AbstractGroupableItem *item);

protected:
    QList<QWeakPointer<Task> > m_tasks;

private:
    void addToTasks(TaskGroup *group);
};

/** Move window to current desktop*/
class ToCurrentDesktopActionImpl : public AbstractGroupableItemAction
{
    Q_OBJECT
public:
    ToCurrentDesktopActionImpl(QObject *parent, AbstractGroupableItem *task);
private Q_SLOTS:
    void slotToCurrentDesktop();
};

/** Move window to a new desktop*/
class ToNewDesktopActionImpl : public AbstractGroupableItemAction
{
    Q_OBJECT
public:
    ToNewDesktopActionImpl(QObject *parent, AbstractGroupableItem *task);
private Q_SLOTS:
    void slotToNewDesktop();
private:
    int m_newDesktop;
};

/** Send a Task to a specific Desktop*/
class ToDesktopActionImpl : public AbstractGroupableItemAction
{
    Q_OBJECT
public:
    ToDesktopActionImpl(QObject *parent, AbstractGroupableItem *task, int desktop);
private Q_SLOTS:
    void slotToDesktop();
private:
    int m_desktop;
};


/** Set a window or all windows in a group to FullScreen*/
class ViewFullscreenActionImpl : public QAction
{
    Q_OBJECT
public:
    ViewFullscreenActionImpl(QObject *parent, AbstractGroupableItem *task);
};

/** Keep a Window or all windows in a group above the rest */
class KeepAboveActionImpl : public QAction
{
    Q_OBJECT
public:
    KeepAboveActionImpl(QObject *parent, AbstractGroupableItem *task);
};

/** Keep a Window or all windows in a group below the rest*/
class KeepBelowActionImpl : public QAction
{
    Q_OBJECT
public:
    KeepBelowActionImpl(QObject *parent, AbstractGroupableItem *task);
};


/** Leave current Group*/
class LeaveGroupActionImpl : public QAction
{
    Q_OBJECT
public:
    LeaveGroupActionImpl(QObject *parent, AbstractGroupableItem *task, GroupManager*);

private Q_SLOTS:
    void leaveGroup();

private:
    AbstractGroupableItem *abstractItem;
    GroupManager *groupingStrategy;
};

/** Edit current Group*/
class EditGroupActionImpl : public QAction
{
    Q_OBJECT
public:
    EditGroupActionImpl(QObject *parent, TaskGroup *group, GroupManager*);
};

/** Remove Group
class RemoveGroupActionImpl : public QAction
{
    Q_OBJECT
public:
    RemoveGroupActionImpl(QObject *parent, AbstractGroupableItem *task);
};
*/

} // TaskManager namespace


#endif
