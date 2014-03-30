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

#include "groupmanager.h"

#include <QBuffer>
#include <QList>
#include <QStack>
#include <QTimer>
#include <QUuid>
#include <QFile>
#include <QDebug>

#include <KConfigGroup>
#include <KService>
#include <KSycoca>
#include <KDesktopFile>

#include "abstractsortingstrategy.h"
#include "startup.h"
#include "task.h"
#include "taskitem.h"
#include "taskgroup.h"
#include "taskmanager.h"
#include "strategies/activitysortingstrategy.h"
#include "strategies/alphasortingstrategy.h"
#include "strategies/desktopsortingstrategy.h"
#include "strategies/programgroupingstrategy.h"
#include "strategies/manualgroupingstrategy.h"
#include "strategies/manualsortingstrategy.h"
#include "launcheritem.h"
#include "launcherconfig.h"

namespace TaskManager
{

class GroupManagerPrivate
{
public:
    GroupManagerPrivate(GroupManager *manager)
        : q(manager),
          sortingStrategy(GroupManager::NoSorting),
          groupingStrategy(GroupManager::NoGrouping),
          lastGroupingStrategy(GroupManager::NoGrouping),
          abstractGroupingStrategy(0),
          abstractSortingStrategy(0),
          currentScreen(-1),
          groupIsFullLimit(0),
          showOnlyCurrentDesktop(false),
          showOnlyCurrentActivity(false),
          showOnlyCurrentScreen(false),
          showOnlyMinimized(false),
          onlyGroupWhenFull(false),
          changingGroupingStrategy(false),
          readingLauncherConfig(false),
          separateLaunchers(true),
          forceGrouping(false),
          launchersLocked(false)
    {
    }

    /** reload all tasks from TaskManager */
    void reloadTasks();
    void actuallyReloadTasks();

    /**
    * Keep track of changes in Taskmanager
    */
    void currentDesktopChanged(int);
    void currentActivityChanged(QString);
    void taskChanged(::TaskManager::Task *, ::TaskManager::TaskChanges);
    void checkScreenChange();
    void taskDestroyed(QObject *item);
    void startupItemDestroyed(AbstractGroupableItem *);
    void checkIfFull();
    void actuallyCheckIfFull();
    bool addTask(::TaskManager::Task *);
    void removeTask(::TaskManager::Task *);
    void addStartup(::TaskManager::Startup *);
    void removeStartup(::TaskManager::Startup *);
    void sycocaChanged(const QStringList &types);
    void launcherVisibilityChange();
    void checkLauncherVisibility(LauncherItem *launcher);
    int launcherIndex(const QUrl &url);
    KConfigGroup launcherConfig(const KConfigGroup &config = KConfigGroup());

    TaskGroup *currentRootGroup();

    GroupManager *q;
    QHash<Startup *, TaskItem*> startupList;
    GroupManager::TaskSortingStrategy sortingStrategy;
    GroupManager::TaskGroupingStrategy groupingStrategy;
    GroupManager::TaskGroupingStrategy lastGroupingStrategy;
    AbstractGroupingStrategy *abstractGroupingStrategy;
    AbstractSortingStrategy *abstractSortingStrategy;
    int currentScreen;
    QTimer screenTimer;
    QTimer reloadTimer;
    QTimer checkIfFullTimer;
    QSet<Task *> geometryTasks;
    int groupIsFullLimit;
    QUuid configToken;

    QHash<QString, QHash<int, TaskGroup*> > rootGroups; //container for groups
    QList<LauncherItem *> launchers;
    int currentDesktop;
    QString currentActivity;

    bool showOnlyCurrentDesktop : 1;
    bool showOnlyCurrentActivity : 1;
    bool showOnlyCurrentScreen : 1;
    bool showOnlyMinimized : 1;
    bool onlyGroupWhenFull : 1;
    bool changingGroupingStrategy : 1;
    bool readingLauncherConfig : 1;
    bool separateLaunchers : 1;
    bool forceGrouping : 1;
    bool launchersLocked : 1;
};


GroupManager::GroupManager(QObject *parent)
    : QObject(parent),
      d(new GroupManagerPrivate(this))
{
    connect(TaskManager::self(), SIGNAL(taskAdded(::TaskManager::Task *)), this, SLOT(addTask(::TaskManager::Task *)));
    connect(TaskManager::self(), SIGNAL(taskRemoved(::TaskManager::Task *)), this, SLOT(removeTask(::TaskManager::Task *)));
    connect(TaskManager::self(), SIGNAL(startupAdded(::TaskManager::Startup *)), this, SLOT(addStartup(::TaskManager::Startup *)));
    connect(TaskManager::self(), SIGNAL(startupRemoved(::TaskManager::Startup *)), this, SLOT(removeStartup(::TaskManager::Startup *)));
    connect(KSycoca::self(), SIGNAL(databaseChanged(QStringList)), this, SLOT(sycocaChanged(const QStringList &)));

    d->currentDesktop = TaskManager::self()->currentDesktop();
    d->currentActivity = TaskManager::self()->currentActivity();

    d->rootGroups[d->currentActivity][d->currentDesktop] = new TaskGroup(this, "RootGroup");

    d->reloadTimer.setSingleShot(true);
    d->reloadTimer.setInterval(0);
    connect(&d->reloadTimer, SIGNAL(timeout()), this, SLOT(actuallyReloadTasks()));

    d->screenTimer.setSingleShot(true);
    d->screenTimer.setInterval(100);
    connect(&d->screenTimer, SIGNAL(timeout()), this, SLOT(checkScreenChange()));

    d->checkIfFullTimer.setSingleShot(true);
    d->checkIfFullTimer.setInterval(0);
    connect(&d->checkIfFullTimer, SIGNAL(timeout()), this, SLOT(actuallyCheckIfFull()));
}

GroupManager::~GroupManager()
{
    TaskManager::self()->setTrackGeometry(false, d->configToken);
    delete d->abstractSortingStrategy;
    delete d->abstractGroupingStrategy;
    delete d;
}

TaskGroup *GroupManagerPrivate::currentRootGroup()
{
    return rootGroups[currentActivity][currentDesktop];
}

void GroupManagerPrivate::reloadTasks()
{
    reloadTimer.start();
}

void GroupManagerPrivate::actuallyReloadTasks()
{
    //kDebug() << "number of tasks available " << TaskManager::self()->tasks().size();
    QHash<WId, Task *> taskList = TaskManager::self()->tasks();
    QMutableHashIterator<WId, Task *> it(taskList);

    while (it.hasNext()) {
        it.next();

        if (addTask(it.value())) {
            //kDebug() << "task added " << it.value()->visibleName();
            it.remove();
        }
    }

    // Remove what remains
    it.toFront();
    while (it.hasNext()) {
        it.next();
        removeTask(it.value());
    }

    emit q->reload();
}

void GroupManagerPrivate::addStartup(::TaskManager::Startup *task)
{
    //kDebug();
    if (!startupList.contains(task)) {
        TaskItem *item = new TaskItem(q, task);
        startupList.insert(task, item);
        currentRootGroup()->add(item);
        QObject::connect(item, SIGNAL(destroyed(AbstractGroupableItem*)),
                         q, SLOT(startupItemDestroyed(AbstractGroupableItem*)));
    }
}

void GroupManagerPrivate::removeStartup(::TaskManager::Startup *task)
{
    //kDebug();
    if (!startupList.contains(task)) {
        qWarning() << "invalid startup task";
        return;
    }

    TaskItem *item = startupList.take(task);
    if (item->parentGroup()) {
        item->parentGroup()->remove(item);
    }

    item->setTaskPointer(0);
}

bool GroupManagerPrivate::addTask(::TaskManager::Task *task)
{
    if (!task) {
        return false;
    }

    //kDebug();
    /* kDebug() << task->visibleName()
             << task->visibleNameWithState()
             << task->name()
             << task->className()
             << task->classClass(); */

    bool skip = false;

    if (!task->showInTaskbar()) {
        //kDebug() << "Do not show in taskbar";
        skip = true;
    }

    if (showOnlyCurrentScreen && !task->isOnScreen(currentScreen)) {
        //kDebug() << "Not on this screen and showOnlyCurrentScreen";
        skip = true;
    }

    // Should the Task be displayed ? We always display if attention is demaded
    if (!task->demandsAttention()) {
        // As the Task doesn't demand attention
        // go through all filters whether the task should be displayed or not
        if (showOnlyCurrentDesktop && !task->isOnCurrentDesktop()) {
            /* kDebug() << "Not on this desktop and showOnlyCurrentDesktop"
                     << KWindowSystem::currentDesktop() << task->desktop(); */
            skip = true;
        }

        if (showOnlyCurrentActivity && !task->isOnCurrentActivity()) {
            /* kDebug() << "Not on this desktop and showOnlyCurrentActivity"
                     << KWindowSystem::currentActivity() << task->desktop(); */
            skip = true;
        }

        if (showOnlyMinimized && !task->isMinimized()) {
            //kDebug() << "Not minimized and only showing minimized";
            skip = true;
        }

        NET::WindowType type = task->info().windowType(NET::NormalMask | NET::DialogMask |
                               NET::OverrideMask | NET::UtilityMask);
        if (type == NET::Utility) {
            //kDebug() << "skipping utility window" << task->name();
            skip = true;
        }

        //TODO: should we check for transiency? if so the following code can detect it.
        /*
            QHash <Task *, TaskItem*>::iterator it = d->itemList.begin();

            while (it != d->itemList.end()) {
                TaskItem *item = it.value();
                if (item->task()->hasTransient(task->window())) {
                    kDebug() << "TRANSIENT TRANSIENT TRANSIENT!";
                    return flase;
                }
                ++it;
            }
        */
    }

    //Ok the Task should be displayed
    TaskItem *item = qobject_cast<TaskItem*>(currentRootGroup()->getMemberByWId(task->window()));
    if (!item || skip) {
        TaskItem *startupItem = 0;
        QHash<Startup *, TaskItem *>::iterator it = startupList.begin();
        QHash<Startup *, TaskItem *>::iterator itEnd = startupList.end();
        while (it != itEnd) {
            if (it.key()->matchesWindow(task->window())) {
                //kDebug() << "startup task found";
                item = startupItem = it.value();
                startupList.erase(it);
                QObject::disconnect(item, 0, q, 0);
                if (!skip) {
                    item->setTaskPointer(task);
                }
                break;
            }
            ++it;
        }

        // if we are to skip because we don't display, we simply delete the startup related to it
        if (skip) {
            delete startupItem;
            return false;
        }

        if (!item) {
            item = new TaskItem(q, task);
        }

        QObject::connect(task, SIGNAL(destroyed(QObject*)), q, SLOT(taskDestroyed(QObject*)));

        foreach (LauncherItem * launcher, launchers) {
            if (launcher->associateItemIfMatches(item)) {
                // Task demands attention, so is to be shown, therefore hide the launcher...
                currentRootGroup()->remove(launcher);
            }
        }
    }

    //Find a fitting group for the task with GroupingStrategies
    if (abstractGroupingStrategy && (forceGrouping || !task->demandsAttention())) { //do not group attention tasks
        abstractGroupingStrategy->handleItem(item);
    } else {
        currentRootGroup()->add(item);

        if (abstractSortingStrategy) {
            abstractSortingStrategy->handleItem(item);
            abstractSortingStrategy->check(item);
        }
    }

    if (showOnlyCurrentScreen) {
        geometryTasks.insert(task);
    }

    return true;
}

void GroupManagerPrivate::removeTask(::TaskManager::Task *task)
{
    if (!task) {
        return;
    }

    //kDebug() << "remove: " << task->visibleName();
    geometryTasks.remove(task);

    AbstractGroupableItem *item = currentRootGroup()->getMemberByWId(task->window());
    if (!item) {
        // this can happen if the window hasn't been caught previously,
        // of it it is an ignored type such as a NET::Utility type window
        //kDebug() << "invalid item";
        return;
    }

    foreach (LauncherItem * launcher, launchers) {
        launcher->removeItemIfAssociated(item);
    }

    if (item->parentGroup()) {
        item->parentGroup()->remove(item);
    }

    //the item must exist as long as the Task does because of activate calls so don't delete the item here, it will delete itself.
}

void GroupManagerPrivate::taskDestroyed(QObject *item)
{
    Task *task = static_cast<Task*>(item);
    if (showOnlyCurrentScreen) {
        geometryTasks.remove(task);
    }
}

void GroupManagerPrivate::startupItemDestroyed(AbstractGroupableItem *item)
{
    TaskItem *taskItem = static_cast<TaskItem*>(item);
    startupList.remove(startupList.key(taskItem));
    geometryTasks.remove(taskItem->task());
}

bool GroupManager::manualGroupingRequest(AbstractGroupableItem* item, TaskGroup* groupItem)
{
    if (d->abstractGroupingStrategy) {
        return d->abstractGroupingStrategy->manualGroupingRequest(item, groupItem);
    }
    return false;
}

bool GroupManager::manualGroupingRequest(ItemList items)
{
    if (d->abstractGroupingStrategy) {
        return d->abstractGroupingStrategy->manualGroupingRequest(items);
    }
    return false;
}

bool GroupManager::manualSortingRequest(AbstractGroupableItem* taskItem, int newIndex)
{
    if (d->abstractSortingStrategy &&
            (!launchersLocked() || separateLaunchers() || newIndex >= launcherCount())) {
        return d->abstractSortingStrategy->manualSortingRequest(taskItem, newIndex);
    }
    return false;
}


GroupPtr GroupManager::rootGroup() const
{
    return d->currentRootGroup();
}

void GroupManagerPrivate::currentActivityChanged(QString newActivity)
{
    if (!showOnlyCurrentActivity || currentActivity == newActivity) {
        return;
    }

    if (!rootGroups.contains(newActivity) || !rootGroups.value(newActivity).contains(currentDesktop)) {
        qDebug() << "created new desk group";
        rootGroups[newActivity][currentDesktop] = new TaskGroup(q, "RootGroup");
        if (abstractSortingStrategy) {
            abstractSortingStrategy->handleGroup(rootGroups[newActivity][currentDesktop]);
        }
    }

    if (onlyGroupWhenFull) {
        QObject::disconnect(currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
        QObject::disconnect(currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
    }

    currentActivity = newActivity;

    foreach (LauncherItem * item, launchers) {
        if (item->shouldShow(q)) {
            rootGroups[currentActivity][currentDesktop]->add(item);
        } else {
            rootGroups[currentActivity][currentDesktop]->remove(item);
        }
    }

    if (onlyGroupWhenFull) {
        QObject::connect(currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
        QObject::connect(currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
    }

    actuallyReloadTasks();
}

void GroupManagerPrivate::currentDesktopChanged(int newDesktop)
{
    //kDebug();
    if (!showOnlyCurrentDesktop) {
        return;
    }

    if (currentDesktop == newDesktop) {
        return;
    }

    if (!rootGroups[currentActivity].contains(newDesktop)) {
        qDebug() << "created new desk group";
        rootGroups[currentActivity][newDesktop] = new TaskGroup(q, "RootGroup");
        if (abstractSortingStrategy) {
            abstractSortingStrategy->handleGroup(rootGroups[currentActivity][newDesktop]);
        }
    }

    if (onlyGroupWhenFull) {
        QObject::disconnect(currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
        QObject::disconnect(currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
    }

    currentDesktop = newDesktop;

    foreach (LauncherItem * item, launchers) {
        if (item->shouldShow(q)) {
            rootGroups[currentActivity][currentDesktop]->add(item);
        } else {
            rootGroups[currentActivity][currentDesktop]->remove(item);
        }
    }

    if (onlyGroupWhenFull) {
        QObject::connect(currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
        QObject::connect(currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), q, SLOT(checkIfFull()));
    }

    actuallyReloadTasks();
}


void GroupManagerPrivate::taskChanged(::TaskManager::Task *task, ::TaskManager::TaskChanges changes)
{
    //kDebug();
    if (!task) {
        return;
    }

    bool takeAction = false;
    bool show = true;

    if (showOnlyCurrentDesktop && changes & ::TaskManager::DesktopChanged) {
        takeAction = true;
        show = task->isOnCurrentDesktop();
        //kDebug() << task->visibleName() << "on" << TaskManager::self()->currentDesktop();
    }

    if (showOnlyCurrentActivity && changes & ::TaskManager::ActivitiesChanged) {
        takeAction = true;
        show = task->isOnCurrentActivity();
        //kDebug() << task->visibleName() << "on" << TaskManager::self()->currentDesktop();
    }

    if (showOnlyMinimized && changes & ::TaskManager::StateChanged) {
        //TODO: wouldn't it be nice to get notification of JUST minimization?
        takeAction = true;
        show = task->isMinimized();
    }

    if (showOnlyCurrentScreen && changes & ::TaskManager::GeometryChanged) {
        geometryTasks.insert(task);

        if (!screenTimer.isActive()) {
            screenTimer.start();
        }
    }

    if (changes & ::TaskManager::AttentionChanged) {
        // we show tasks anyway if they demand attention
        // so whenever our state changes ... try to re-adjust it
        takeAction = true;
        show = true;
    }

    // Some apps, eg. LibreOffice, change classClass/className after start-up...
    if (changes & ::TaskManager::ClassChanged) {
        // Instead of just moving  item (as what happend in the #if below), just remove and re-add the task.
        // This way the grouping happens properly.
        AbstractGroupableItem *item = currentRootGroup()->getMemberByWId(task->window());
        if (item && TaskItemType == item->itemType()) {
            static_cast<TaskItem *>(item)->resetLauncherCheck();
        }
        removeTask(task);
        addTask(task);
    }

    if (!takeAction) {
        return;
    }

    show = show && (!showOnlyCurrentScreen || task->isOnScreen(currentScreen));

    if (show) {
        //kDebug() << "add(task);";
        addTask(task);
    } else {
        //kDebug() << "remove(task);";
        removeTask(task);
    }
}

void GroupManager::setScreen(int screen)
{
    if (screen != d->currentScreen) {
        d->currentScreen = screen;
        d->reloadTasks();
        emit screenChanged(screen);
    }
}

int GroupManager::screen() const
{
    return d->currentScreen;
}

void GroupManagerPrivate::checkScreenChange()
{
    //kDebug();
    if (showOnlyCurrentScreen) {
        foreach (Task *task, geometryTasks) {
            if (task->isOnScreen(currentScreen)) {
                addTask(task);
            } else {
                removeTask(task);
            }
        }
    }

    geometryTasks.clear();
}

void GroupManager::reconnect()
{
    //kDebug();
    disconnect(TaskManager::self(), SIGNAL(desktopChanged(int)), this, SLOT(currentDesktopChanged(int)));
    disconnect(TaskManager::self(), SIGNAL(activityChanged(QString)), this, SLOT(currentActivityChanged(QString)));
    disconnect(TaskManager::self(), SIGNAL(windowChanged(::TaskManager::Task *, ::TaskManager::TaskChanges)),
               this, SLOT(taskChanged(::TaskManager::Task *, ::TaskManager::TaskChanges)));

    if (d->showOnlyCurrentDesktop || d->showOnlyMinimized || d->showOnlyCurrentScreen || d->showOnlyCurrentActivity) {
        // listen to the relevant task manager signals
        if (d->showOnlyCurrentDesktop) {
            connect(TaskManager::self(), SIGNAL(desktopChanged(int)),
                    this, SLOT(currentDesktopChanged(int)));
        }
        if (d->showOnlyCurrentActivity) {
            connect(TaskManager::self(), SIGNAL(activityChanged(QString)),
                    this, SLOT(currentActivityChanged(QString)));
        }

        connect(TaskManager::self(), SIGNAL(windowChanged(::TaskManager::Task *, ::TaskManager::TaskChanges)),
                this, SLOT(taskChanged(::TaskManager::Task *, ::TaskManager::TaskChanges)));
    }

    TaskManager::self()->setTrackGeometry(d->showOnlyCurrentScreen, d->configToken);

    if (!d->showOnlyCurrentScreen) {
        d->geometryTasks.clear();
    }

    d->reloadTasks();
}

bool GroupManager::addLauncher(const QUrl &url, const QIcon &icon, const QString &name, const QString &genericName, const QString &wmClass, int insertPos)
{
    if (url.isEmpty() || launchersLocked()) {
        return false;
    }

    int index = launcherIndex(url);

    LauncherItem *launcher = -1 != index ? d->launchers.at(index) : 0L; // Do not insert launchers twice

    if (!launcher) {
        launcher = new LauncherItem(d->currentRootGroup(), url);

        if (!launcher->isValid()) {
            delete launcher;
            return false;
        }

        if (!icon.isNull()) {
            launcher->setIcon(icon);
        }

        if (!name.isEmpty()) {
            launcher->setName(name);
        }

        if (!genericName.isEmpty()) {
            launcher->setGenericName(genericName);
        }

        if (!wmClass.isEmpty()) {
            launcher->setWmClass(wmClass);
        }

        QStack<TaskGroup *> groups;
        groups.push(d->currentRootGroup());
        while (!groups.isEmpty()) {
            TaskGroup *group = groups.pop();

            foreach (AbstractGroupableItem * item, group->members()) {
                if (item->itemType() == GroupItemType) {
                    groups.push(static_cast<TaskGroup *>(item));
                } else {
                    launcher->associateItemIfMatches(item);
                }
            }
        }

        if (insertPos >= 0 && insertPos < d->launchers.count()) {
            d->launchers.insert(insertPos, launcher);
        } else {
            d->launchers.append(launcher);
        }

        connect(launcher, SIGNAL(associationChanged()), this, SLOT(launcherVisibilityChange()));
        d->checkLauncherVisibility(launcher);

        if (!d->separateLaunchers && d->abstractSortingStrategy && ManualSorting == d->abstractSortingStrategy->type()) {
            // Ensure item is placed where launcher would be...
            foreach (AbstractGroupableItem * item, d->rootGroups[d->currentActivity][d->currentDesktop]->members()) {
                if (LauncherItemType != item->itemType() && item->launcherUrl() == url) {
                    manualSortingRequest(item, launcherIndex(url));
                    break;
                }
            }
        }

        if (!d->readingLauncherConfig) {
            emit launcherListChanged();
        }
    }

    return launcher;
}

void GroupManager::removeLauncher(const QUrl &url)
{
    if (launchersLocked()) {
        return;
    }

    int index = launcherIndex(url);
    if (index == -1 || index >= d->launchers.count()) {
        return;
    }

    LauncherItem *launcher = d->launchers.at(index);
    if (!launcher) {
        return;
    }

    d->launchers.removeAt(index);

    typedef QHash<int, TaskGroup*> Metagroup;
    foreach (Metagroup metagroup, d->rootGroups) {
        foreach (TaskGroup * rootGroup, metagroup) {
            rootGroup->remove(launcher);
        }
    }

    launcher->deleteLater();

    if (!d->separateLaunchers && d->abstractSortingStrategy && ManualSorting == d->abstractSortingStrategy->type()) {
        // Ensure item is placed at end of launchers...
        foreach (AbstractGroupableItem * item, d->rootGroups[d->currentActivity][d->currentDesktop]->members()) {
            if (LauncherItemType != item->itemType() && item->launcherUrl() == url) {
                manualSortingRequest(item, d->launchers.count());
                break;
            }
        }
    }

    if (!d->readingLauncherConfig) {
        emit launcherListChanged();
    }
}

void GroupManagerPrivate::sycocaChanged(const QStringList &types)
{
    if (types.contains("apps")) {
        QList<QUrl> removals;
        foreach (LauncherItem *launcher, launchers) {
            if (launcher->launcherUrl().scheme() != "preferred" &&
                !QFile::exists(launcher->launcherUrl().toLocalFile())) {
                removals << launcher->launcherUrl();
            }
        }

        foreach (const QUrl & url, removals) {
            q->removeLauncher(url);
        }
    }
}

void GroupManagerPrivate::launcherVisibilityChange()
{
    checkLauncherVisibility(qobject_cast<LauncherItem *>(q->sender()));
}

void GroupManagerPrivate::checkLauncherVisibility(LauncherItem *launcher)
{
    if (!launcher) {
        return;
    }

    if (launcher->shouldShow(q)) {
        rootGroups[currentActivity][currentDesktop]->add(launcher);
    } else {
        rootGroups[currentActivity][currentDesktop]->remove(launcher);
    }
}

bool GroupManager::launcherExists(const QUrl &url) const
{
    return d->launcherIndex(url) != -1;
}

int GroupManager::launcherIndex(const QUrl &url) const
{
    return d->launcherIndex(url);
}

int GroupManager::launcherCount() const
{
    return d->launchers.count();
}

bool GroupManager::launchersLocked() const
{
    return d->launchersLocked;
}

void GroupManager::setLaunchersLocked(bool l)
{
    d->launchersLocked = l;
}

QUrl GroupManager::launcherForWmClass(const QString &wmClass) const
{
    foreach (LauncherItem * l, d->launchers) {
        if (l->wmClass() == wmClass) {
            return l->launcherUrl();
        }
    }

    return QUrl();
}

QString GroupManager::launcherWmClass(const QUrl &url) const
{
    int index = launcherIndex(url);
    LauncherItem *l = -1 != index ? d->launchers.at(index) : 0L;
    return l ? l->wmClass() : QString();
}

bool GroupManager::isItemAssociatedWithLauncher(AbstractGroupableItem *item) const
{
    if (item) {
        switch (item->itemType()) {
        case LauncherItemType:
            return true;
        case GroupItemType: {
            foreach (AbstractGroupableItem * i, static_cast<TaskGroup *>(item)->members()) {
                if (isItemAssociatedWithLauncher(i)) {
                    return true;
                }
            }
            break;
        }
        case TaskItemType: {
            foreach (LauncherItem * launcher, d->launchers) {
                if (launcher->isAssociated(item)) {
                    return true;
                }
            }
        }
        }
    }

    return false;
}

void GroupManager::moveLauncher(const QUrl &url, int newIndex)
{
    if (!url.isValid()) {
        return;
    }
    int oldIndex = launcherIndex(url);

    if (oldIndex >= 0 && newIndex != oldIndex) {
        d->launchers.insert(newIndex, d->launchers.takeAt(oldIndex));
        emit launcherListChanged();
    }
}

bool GroupManager::separateLaunchers() const
{
    return d->separateLaunchers;
}

void GroupManager::setSeparateLaunchers(bool s)
{
    d->separateLaunchers = s;
}

bool GroupManager::forceGrouping() const
{
    return d->forceGrouping;
}

void GroupManager::setForceGrouping(bool s)
{
    d->forceGrouping = s;
}

void GroupManager::createConfigurationInterface(KConfigDialog *parent)
{
    new LauncherConfig(parent);
}

QList<QUrl> GroupManager::launcherList() const
{
    QList<QUrl> launchers;

    foreach (LauncherItem *l, d->launchers) {
        QUrl u(l->launcherUrl());

        if (!l->wmClass().isEmpty()) {
            u.addQueryItem("wmClass", l->wmClass());
        }

        if (!l->launcherUrl().isValid() || !KDesktopFile::isDesktopFile(l->launcherUrl().toLocalFile())) {
            if (!l->name().isEmpty()) {
                u.addQueryItem("name", l->name());
            }

            if (!l->genericName().isEmpty()) {
                u.addQueryItem("genericName", l->genericName());
            }

            if (!l->icon().name().isEmpty()) {
                u.addQueryItem("icon", l->icon().name());
            } else if (!l->icon().isNull()) {
                QPixmap pixmap = l->icon().pixmap(QSize(64, 64));
                QByteArray bytes;
                QBuffer buffer(&bytes);
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG");
                u.addQueryItem("iconData", bytes.toBase64(QByteArray::Base64UrlEncoding));
            }
        }

        launchers << u;
    }

    return launchers;
}

void GroupManager::setLauncherList(QList<QUrl> launcherList)
{
    d->readingLauncherConfig = true;

    QSet<QUrl> urls;

    foreach (QUrl l, launcherList) {
        QUrlQuery query(l);

        QString name(query.queryItemValue("name"));
        QString genericName(query.queryItemValue("genericName"));
        QString wmClass(query.queryItemValue("wmClass"));
        QString iconData(query.queryItemValue("iconData"));

        QIcon icon;

        if (!iconData.isEmpty()) {
            QPixmap pixmap;
            QByteArray bytes = QByteArray::fromBase64(iconData.toLocal8Bit(), QByteArray::Base64UrlEncoding);
            pixmap.loadFromData(bytes);
            icon.addPixmap(pixmap);
        } else {
            icon = QIcon::fromTheme(query.queryItemValue("icon"));
        }

        l.setQuery(QUrlQuery());

        if (addLauncher(l, QIcon(), name, genericName, wmClass)) {
            urls << l;
        }
    }

    QList<QUrl> removals;

    foreach (LauncherItem *launcher, d->launchers) {
        if (!urls.contains(launcher->launcherUrl())) {
            removals << launcher->launcherUrl();
        }
    }

    foreach (const QUrl & url, removals) {
        removeLauncher(url);
    }

    d->readingLauncherConfig = false;

    emit launcherListChanged();
}

int GroupManagerPrivate::launcherIndex(const QUrl &url)
{
    // we check first for exact matches ...
    int index = 0;
    foreach (const LauncherItem * item, launchers) {
        if (item->launcherUrl() == url) {
            return index;
        }

        ++index;
    }

    // .. and if that fails for preferred launcher matches
    index = 0;
    foreach (const LauncherItem * item, launchers) {
        if (item->launcherUrl().scheme() == "preferred") {
            KService::Ptr service = KService::serviceByStorageId(item->defaultApplication());

            if (service) {
                QUrl prefUrl(service->entryPath());
                if (prefUrl.scheme().isEmpty()) {
                    prefUrl.setScheme("file");
                }

                if (prefUrl == url) {
                    return index;
                }
            }
        }

        ++index;
    }

    return -1;
}

bool GroupManager::onlyGroupWhenFull() const
{
    return d->onlyGroupWhenFull;
}

void GroupManager::setOnlyGroupWhenFull(bool onlyGroupWhenFull)
{
    //kDebug() << onlyGroupWhenFull;
    if (d->onlyGroupWhenFull == onlyGroupWhenFull) {
        return;
    }

    d->onlyGroupWhenFull = onlyGroupWhenFull;

    disconnect(d->currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), this, SLOT(checkIfFull()));
    disconnect(d->currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), this, SLOT(checkIfFull()));

    if (onlyGroupWhenFull) {
        connect(d->currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), this, SLOT(checkIfFull()));
        connect(d->currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), this, SLOT(checkIfFull()));
        d->checkIfFull();
    } else {
        setGroupingStrategy(d->groupingStrategy);
    }

    emit onlyGroupWhenFullChanged(onlyGroupWhenFull);
}

int GroupManager::fullLimit() const
{
    return d->groupIsFullLimit;
}

void GroupManager::setFullLimit(int limit)
{
    //kDebug() << limit;
    if (!d->groupIsFullLimit != limit) {
        d->groupIsFullLimit = limit;

        if (d->onlyGroupWhenFull) {
            d->checkIfFull();
        }

        emit fullLimitChanged(limit);
    }
}

void GroupManagerPrivate::checkIfFull()
{
    // Start a timer so that if we have been triggered by a layouting
    // we give time for it to finish up instead of starting a new one
    // right away.
    checkIfFullTimer.start();
}

void GroupManagerPrivate::actuallyCheckIfFull()
{
    //kDebug();
    if (!onlyGroupWhenFull ||
            groupingStrategy != GroupManager::ProgramGrouping ||
            changingGroupingStrategy) {
        return;
    }

    if (currentRootGroup()->totalSize() >= groupIsFullLimit) {
        if (!abstractGroupingStrategy) {
            geometryTasks.clear();
            q->setGroupingStrategy(GroupManager::ProgramGrouping);
        }
    } else if (abstractGroupingStrategy) {
        geometryTasks.clear();
        q->setGroupingStrategy(GroupManager::NoGrouping);
        //let the visualization think we still use the programGrouping
        groupingStrategy = GroupManager::ProgramGrouping;
    }
}

bool GroupManager::showOnlyCurrentScreen() const
{
    return d->showOnlyCurrentScreen;
}

void GroupManager::setShowOnlyCurrentScreen(bool showOnlyCurrentScreen)
{
    if (showOnlyCurrentScreen != d->showOnlyCurrentScreen) {
        d->showOnlyCurrentScreen = showOnlyCurrentScreen;
        reconnect();
        emit showOnlyCurrentScreenChanged(showOnlyCurrentScreen);
    }
}

bool GroupManager::showOnlyCurrentDesktop() const
{
    return d->showOnlyCurrentDesktop;
}

void GroupManager::setShowOnlyCurrentDesktop(bool showOnlyCurrentDesktop)
{
    if (showOnlyCurrentDesktop != d->showOnlyCurrentDesktop) {
        d->showOnlyCurrentDesktop = showOnlyCurrentDesktop;
        reconnect();
        emit showOnlyCurrentDesktopChanged(showOnlyCurrentDesktop);
    }
}

bool GroupManager::showOnlyCurrentActivity() const
{
    return d->showOnlyCurrentActivity;
}

void GroupManager::setShowOnlyCurrentActivity(bool showOnlyCurrentActivity)
{
    if (showOnlyCurrentActivity != d->showOnlyCurrentActivity) {
        d->showOnlyCurrentActivity = showOnlyCurrentActivity;
        reconnect();
        emit showOnlyCurrentActivityChanged(showOnlyCurrentActivity);
    }
}

bool GroupManager::showOnlyMinimized() const
{
    return d->showOnlyMinimized;
}

void GroupManager::setShowOnlyMinimized(bool showOnlyMinimized)
{
    if (showOnlyMinimized != d->showOnlyMinimized) {
        d->showOnlyMinimized = showOnlyMinimized;
        reconnect();
        emit showOnlyMinimizedChanged(showOnlyMinimized);
    }
}

GroupManager::TaskSortingStrategy GroupManager::sortingStrategy() const
{
    return d->sortingStrategy;
}

AbstractSortingStrategy* GroupManager::taskSorter() const
{
    return d->abstractSortingStrategy;
}

void GroupManager::setSortingStrategy(TaskSortingStrategy sortOrder)
{
    if (d->abstractSortingStrategy) {
        if (d->abstractSortingStrategy->type() == sortOrder) {
            return;
        }

        d->abstractSortingStrategy->deleteLater();
        d->abstractSortingStrategy = 0;
    }

    switch (sortOrder) {
    case ManualSorting:
        d->abstractSortingStrategy = new ManualSortingStrategy(this);
        break;

    case AlphaSorting:
        d->abstractSortingStrategy = new AlphaSortingStrategy(this);
        break;

    case DesktopSorting:
        d->abstractSortingStrategy = new DesktopSortingStrategy(this);
        break;

    case ActivitySorting:
        d->abstractSortingStrategy = new ActivitySortingStrategy(this);
        break;

    case NoSorting: //manual and no grouping result both in non automatic grouping
        break;

    default:
        qDebug() << "Invalid Strategy";
    }
    if (d->abstractSortingStrategy) {
        typedef QHash<int, TaskGroup*> Metagroup;
        foreach (Metagroup metagroup, d->rootGroups) {
            foreach (TaskGroup * group, metagroup) {
                d->abstractSortingStrategy->handleGroup(group);
            }
        }
    }

    d->sortingStrategy = sortOrder;
    reconnect();

    emit sortingStrategyChanged(sortOrder);
}

GroupManager::TaskGroupingStrategy GroupManager::groupingStrategy() const
{
    return d->groupingStrategy;
}

AbstractGroupingStrategy* GroupManager::taskGrouper() const
{
    return d->abstractGroupingStrategy;
}


void GroupManager::setGroupingStrategy(TaskGroupingStrategy strategy)
{
    if (d->changingGroupingStrategy || d->groupingStrategy == strategy) {
        return;
    }

    d->changingGroupingStrategy = true;

    //kDebug() << strategy << kBacktrace();
    if (d->onlyGroupWhenFull) {
        disconnect(d->currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), this, SLOT(checkIfFull()));
        disconnect(d->currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), this, SLOT(checkIfFull()));
    }

    if (d->abstractGroupingStrategy) {
        disconnect(d->abstractGroupingStrategy, 0, this, 0);
        d->abstractGroupingStrategy->destroy();
        d->abstractGroupingStrategy = 0;
    }

    switch (strategy) {
    case ManualGrouping:
        d->abstractGroupingStrategy = new ManualGroupingStrategy(this);
        break;

    case ProgramGrouping:
        d->abstractGroupingStrategy = new ProgramGroupingStrategy(this);
        break;

    case NoGrouping:
        break;

    default:
        qDebug() << "Strategy not implemented";
    }

    d->groupingStrategy = strategy;

    d->actuallyReloadTasks();

    if (d->onlyGroupWhenFull) {
        connect(d->currentRootGroup(), SIGNAL(itemAdded(AbstractGroupableItem*)), this, SLOT(checkIfFull()));
        connect(d->currentRootGroup(), SIGNAL(itemRemoved(AbstractGroupableItem*)), this, SLOT(checkIfFull()));
    }

    d->changingGroupingStrategy = false;

    emit groupingStrategyChanged(strategy);
}

} // TaskManager namespace

#include "moc_groupmanager.cpp"
