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

#ifndef GROUPMANAGER_H
#define GROUPMANAGER_H

#include <QtCore/QObject>

#include <abstractgroupableitem.h>
#include <task.h>
#include <taskitem.h>
#include <taskmanager_export.h>
#include "launcheritem.h"

class KConfigDialog;

namespace TaskManager
{

class AbstractSortingStrategy;
class AbstractGroupingStrategy;
class GroupManagerPrivate;

/**
 * Manages the grouping stuff. It doesn't know anything about grouping and sorting itself, this is done in the grouping and sorting strategies.
 */
class TASKMANAGER_EXPORT GroupManager: public QObject
{

    Q_OBJECT

    Q_PROPERTY(int screen READ screen WRITE setScreen NOTIFY screenChanged)
    Q_PROPERTY(TaskGroupingStrategy groupingStrategy READ groupingStrategy WRITE setGroupingStrategy NOTIFY groupingStrategyChanged)
    Q_PROPERTY(bool onlyGroupWhenFull READ onlyGroupWhenFull WRITE setOnlyGroupWhenFull NOTIFY onlyGroupWhenFullChanged)
    Q_PROPERTY(int fullLimit READ fullLimit WRITE setFullLimit NOTIFY fullLimitChanged)
    Q_PROPERTY(TaskSortingStrategy sortingStrategy READ sortingStrategy WRITE setSortingStrategy NOTIFY sortingStrategyChanged)
    Q_PROPERTY(bool showOnlyCurrentScreen READ showOnlyCurrentScreen WRITE setShowOnlyCurrentScreen NOTIFY showOnlyCurrentScreenChanged)
    Q_PROPERTY(bool showOnlyCurrentDesktop READ showOnlyCurrentDesktop WRITE setShowOnlyCurrentDesktop NOTIFY showOnlyCurrentDesktopChanged)
    Q_PROPERTY(bool showOnlyCurrentActvity READ showOnlyCurrentActivity WRITE setShowOnlyCurrentActivity NOTIFY showOnlyCurrentActivityChanged)
    Q_PROPERTY(bool showOnlyMinimized READ showOnlyMinimized WRITE setShowOnlyMinimized NOTIFY showOnlyMinimizedChanged)
    Q_PROPERTY(QList<QUrl> launcherList READ launcherList WRITE setLauncherList NOTIFY launcherListChanged)

public:
    GroupManager(QObject *parent);
    ~GroupManager();

    /**
    * Returns a group which contains all items and subgroups.
    * Visualizations should be based on this.
    */
    GroupPtr rootGroup() const;

    /**
    * Strategy used to Group new items
    */
    enum TaskGroupingStrategy {
        NoGrouping = 0,
        ManualGrouping = 1, //Allow manual grouping
        ProgramGrouping = 2
    };

    TaskGroupingStrategy groupingStrategy() const;
    void setGroupingStrategy(TaskGroupingStrategy);
    AbstractGroupingStrategy* taskGrouper() const;


    /**
    * How the task are ordered
    */
    enum TaskSortingStrategy {
        NoSorting = 0,
        ManualSorting = 1,
        AlphaSorting = 2,
        DesktopSorting = 3,
        ActivitySorting = 4
    };

    TaskSortingStrategy sortingStrategy() const;
    void setSortingStrategy(TaskSortingStrategy);
    AbstractSortingStrategy* taskSorter() const;

    bool showOnlyCurrentScreen() const;
    void setShowOnlyCurrentScreen(bool);

    bool showOnlyCurrentDesktop() const;
    void setShowOnlyCurrentDesktop(bool);

    bool showOnlyCurrentActivity() const;
    void setShowOnlyCurrentActivity(bool);

    bool showOnlyMinimized() const;
    void setShowOnlyMinimized(bool);

    bool onlyGroupWhenFull() const;
    /**
    * Only apply the grouping startegy when the taskbar is full according to
    * setFullLimit(int). This is currently limited to ProgramGrouping.
    */
    void setOnlyGroupWhenFull(bool state);
    /**
     * @return the currently set limit when the taskbar is considered as full
     */
    int fullLimit() const;
    /**
    * Set the limit when the taskbar is considered as full
    */
    void setFullLimit(int limit);

    /**
     * Functions to call if the user wants to do something manually, the strategy allows or refuses the request
     */
    bool manualGroupingRequest(AbstractGroupableItem* taskItem, TaskGroup* groupItem);
    bool manualGroupingRequest(ItemList items);

    bool manualSortingRequest(AbstractGroupableItem* taskItem, int newIndex);

    /**
     * The Visualization is responsible to update the screen number the visualization is currently on.
     */
    void setScreen(int screen);

    /**
     * @return the currently set screen; -1 if none
     */
    int screen() const;

    /**
     * Reconnect all neccessary signals to the taskmanger, and clear the per desktop stored rootGroups
     */
    void reconnect();

    /** Adds a Launcher for the executable/.desktop-file at url and returns a reference to the launcher*/
    bool addLauncher(const QUrl &url, const QIcon &icon = QIcon(), const QString &name = QString(),
                     const QString &genericName = QString(), const QString &wmClass = QString(), int insertPos = -1);

    /** Removes the given launcher*/
    void removeLauncher(const QUrl &url);

    /** Get the current list of launchers */
    QList<QUrl> launcherList() const;

    /** Set a new list of launchers */
    void setLauncherList(const QList<QUrl> launchers);

    /** @return true if there is a matching launcher */
    bool launcherExists(const QUrl &url) const;
    bool launcherExistsForUrl(const QUrl &url) const;

    /** @return position of launcher */
    int launcherIndex(const QUrl &url) const;

    /** @return number of launchers */
    int launcherCount() const;

    /** @return true if launchers should not be movable */
    bool launchersLocked() const;

    /** lock launchers, so that they cannot be moved */
    void setLaunchersLocked(bool l);

    /** move a launcher */
    void moveLauncher(const QUrl &url, int newIndex);

    /** should launchers be show separate from tasks? */
    bool separateLaunchers() const;

    /** set if launchers should been show separate from tasks */
    void setSeparateLaunchers(bool s);

    /** Should grouping *always* happen? */
    bool forceGrouping() const;

    /** set if grouping should *always* happen */
    void setForceGrouping(bool s);

    /** create launcher mapping rules config page */
    void createConfigurationInterface(KConfigDialog *parent);

    /** @return the launcher associated with a window class */
    QUrl launcherForWmClass(const QString &wmClass) const;

    /** @return the window class associated with launcher */
    QString launcherWmClass(const QUrl &url) const;

    /** @return true if item is associated with a launcher */
    bool isItemAssociatedWithLauncher(AbstractGroupableItem *item) const;

Q_SIGNALS:
    /** Signal that the rootGroup has to be reloaded in the visualization */
    void reload();

    /** Signal that the launcher list has changed */
    void launcherListChanged();

    void screenChanged(int);
    void groupingStrategyChanged(TaskGroupingStrategy);
    void onlyGroupWhenFullChanged(bool);
    void fullLimitChanged(int);
    void sortingStrategyChanged(TaskSortingStrategy);
    void showOnlyCurrentScreenChanged(bool);
    void showOnlyCurrentDesktopChanged(bool);
    void showOnlyCurrentActivityChanged(bool);
    void showOnlyMinimizedChanged(bool);

private:
    Q_PRIVATE_SLOT(d, void currentDesktopChanged(int))
    Q_PRIVATE_SLOT(d, void currentActivityChanged(QString))
    Q_PRIVATE_SLOT(d, void taskChanged(::TaskManager::Task *, ::TaskManager::TaskChanges))
    Q_PRIVATE_SLOT(d, void checkScreenChange())
    Q_PRIVATE_SLOT(d, void startupItemDestroyed(AbstractGroupableItem *))
    Q_PRIVATE_SLOT(d, void checkIfFull())
    Q_PRIVATE_SLOT(d, void actuallyCheckIfFull())
    Q_PRIVATE_SLOT(d, bool addTask(::TaskManager::Task *))
    Q_PRIVATE_SLOT(d, void removeTask(::TaskManager::Task *))
    Q_PRIVATE_SLOT(d, void addStartup(::TaskManager::Startup *))
    Q_PRIVATE_SLOT(d, void removeStartup(::TaskManager::Startup *))
    Q_PRIVATE_SLOT(d, void actuallyReloadTasks())
    Q_PRIVATE_SLOT(d, void taskDestroyed(QObject *))
    Q_PRIVATE_SLOT(d, void sycocaChanged(const QStringList &))
    Q_PRIVATE_SLOT(d, void launcherVisibilityChange())

    friend class GroupManagerPrivate;
    GroupManagerPrivate * const d;
};

}
#endif
