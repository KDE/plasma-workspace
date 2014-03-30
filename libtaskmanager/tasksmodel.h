/*****************************************************************

Copyright 2010 Aaron Seigo <aseigo@kde.org>
Copyright 2012-2013 Eike Hein <hein@kde.org>

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

*****************************************************************/

#ifndef TASKSMODEL_H
#define TASKSMODEL_H

#include <QAbstractItemModel>

#include "taskmanager_export.h"

namespace TaskManager
{

class GroupManager;
class TaskGroup;
class TasksModelPrivate;

class TASKMANAGER_EXPORT TasksModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_ENUMS(DisplayRoles)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int launcherCount READ launcherCount NOTIFY launcherCountChanged)

public:
    enum DisplayRoles {
        Id = Qt::UserRole + 1,
        GenericName = Qt::UserRole + 2,
        IsStartup = Qt::UserRole + 3,
        IsLauncher = Qt::UserRole + 4,
        OnAllDesktops = Qt::UserRole + 5,
        Desktop = Qt::UserRole + 6,
        DesktopName = Qt::UserRole + 7,
        OnAllActivities = Qt::UserRole + 8,
        ActivityNames = Qt::UserRole + 9,
        OtherActivityNames = Qt::UserRole + 10,
        Shaded = Qt::UserRole + 11,
        Maximized = Qt::UserRole + 12,
        Minimized = Qt::UserRole + 13,
        FullScreen = Qt::UserRole + 14,
        BelowOthers = Qt::UserRole + 15,
        AlwaysOnTop = Qt::UserRole + 16,
        Active = Qt::UserRole + 17,
        DemandsAttention = Qt::UserRole + 18,
        LauncherUrl = Qt::UserRole + 19,
        WindowList = Qt::UserRole + 20,
        MimeType = Qt::UserRole + 21,
        MimeData = Qt::UserRole + 22
    };

    explicit TasksModel(GroupManager *groupManager, QObject *parent = 0);
    ~TasksModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const;
    Q_INVOKABLE int launcherCount() const;

    Q_INVOKABLE int activeTaskId(TaskGroup *group = 0) const;
    Q_INVOKABLE QVariant taskIdList(const QModelIndex &parent = QModelIndex(), bool recursive = true) const;

Q_SIGNALS:
    void countChanged();
    void launcherCountChanged();

private:
    friend class TasksModelPrivate;
    TasksModelPrivate * const d;

    Q_PRIVATE_SLOT(d, void populateModel())
    Q_PRIVATE_SLOT(d, void itemAboutToBeAdded(AbstractGroupableItem *, int))
    Q_PRIVATE_SLOT(d, void itemAdded(AbstractGroupableItem *))
    Q_PRIVATE_SLOT(d, void itemAboutToBeRemoved(AbstractGroupableItem *))
    Q_PRIVATE_SLOT(d, void itemRemoved(AbstractGroupableItem *))
    Q_PRIVATE_SLOT(d, void itemAboutToMove(AbstractGroupableItem *, int, int))
    Q_PRIVATE_SLOT(d, void itemMoved(AbstractGroupableItem *))
    Q_PRIVATE_SLOT(d, void itemChanged(::TaskManager::TaskChanges))
};

} // namespace TaskManager

#endif

