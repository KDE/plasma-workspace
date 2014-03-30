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

#include "taskactions.h"
#include "taskactions_p.h"

// Own

#include "taskgroup.h"
#include "task.h"
#include "taskitem.h"
#include "taskmanager.h"
#include "abstractgroupingstrategy.h"

// KDE
#include <KLocalizedString>
#include <KService>
#include <KServiceTypeTrader>
#include <KDesktopFile>
#include <KRun>
#include <netwm.h>

#include <QtCore/QMap>
#include <QtGui/QHelpEvent>
#include <QtWidgets/QToolTip>
#include <QX11Info>

///////////////////////////////////////////////////////////////////////////////
// NOTE: if you change the menu, keep kde-workspace/kwin/useractions.cpp in sync
//////////////////////////////////////////////////////////////////////////////

namespace TaskManager
{


QAction *standardGroupableAction(GroupableAction action, AbstractGroupableItem *item, GroupManager *strategy, QObject *parent, int desktop)
{
    Q_ASSERT(item);

    switch (action) {
    case MaximizeAction:
        return new MaximizeActionImpl(parent, item);
        break;
    case MinimizeAction:
        return new MinimizeActionImpl(parent, item);
        break;
    case ToCurrentDesktopAction:
        return new ToCurrentDesktopActionImpl(parent, item);
        break;
    case ToDesktopAction:
        return new ToDesktopActionImpl(parent, item, desktop);
        break;
    case ShadeAction:
        return new ShadeActionImpl(parent, item);
        break;
    case CloseAction:
        return new CloseActionImpl(parent, item);
        break;
    case ViewFullscreenAction:
        return new ViewFullscreenActionImpl(parent, item);
        break;
    case KeepBelowAction:
        return new KeepBelowActionImpl(parent, item);
        break;
    case ToggleLauncherAction:
        return new ToggleLauncherActionImpl(parent, item, strategy);
        break;
    case NewInstanceAction:
        return new NewInstanceActionImpl(parent, item);
        break;
    }

    return 0;
}

QAction* standardTaskAction(TaskAction action, TaskItem *item, QObject *parent)
{
    Q_ASSERT(item);

    switch (action) {
    case ResizeAction:
        return new ResizeActionImpl(parent, item);
        break;
    case MoveAction:
        return new MoveActionImpl(parent, item);
        break;
    }

    return 0;
}

QAction* standardGroupingAction(GroupingAction action, AbstractGroupableItem *item, GroupManager *strategy, QObject *parent)
{
    Q_ASSERT(item);
    Q_ASSERT(strategy);

    switch (action) {
    case LeaveGroupAction:
        return new LeaveGroupActionImpl(parent, item, strategy);
        break;
    }

    return 0;
}

static QString qt_strippedText(QString s)
{
    s.remove(QString::fromLatin1("..."));
    int i = 0;
    while (i < s.size()) {
        ++i;
        if (s.at(i - 1) != QLatin1Char('&')) {
            continue;
        }

        if (i < s.size() && s.at(i) == QLatin1Char('&')) {
            ++i;
        }
        s.remove(i - 1, 1);
    }
    return s.trimmed();
}

ToolTipMenu::ToolTipMenu(QWidget *parent, const QString &title)
    : QMenu(title, parent)
{
}

bool ToolTipMenu::event(QEvent* e)
{
    if (QEvent::ToolTip == e->type()) {
        //show action tooltip instead of widget tooltip
        QHelpEvent *he = dynamic_cast<QHelpEvent*>(e);
        QAction *act = he ? actionAt(he->pos()) : 0;
        if (act) {
            if (qt_strippedText(act->text()) != act->toolTip()) {
                QToolTip::showText(he->globalPos(), act->toolTip(), this);
            } else {
                QToolTip::hideText();
            }
            return true;
        }
    }
    return QMenu::event(e);
}

MinimizeActionImpl::MinimizeActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item, SLOT(toggleMinimized()));
    setText(i18n("Mi&nimize"));
    setCheckable(true);
    setChecked(item->isMinimized());
    setEnabled(item->isActionSupported(NET::ActionMinimize));
}


MaximizeActionImpl::MaximizeActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item, SLOT(toggleMaximized()));
    setText(i18n("Ma&ximize"));
    setCheckable(true);
    setChecked(item->isMaximized());
    setEnabled(item->isActionSupported(NET::ActionMax));
}

ShadeActionImpl::ShadeActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item, SLOT(toggleShaded()));
    setText(i18n("&Shade"));
    setCheckable(true);
    setChecked(item->isShaded());
    setEnabled(item->isActionSupported(NET::ActionShade));
}

ResizeActionImpl::ResizeActionImpl(QObject *parent, TaskItem* item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item->task(), SLOT(resize()));
    setText(i18n("Re&size"));
    setEnabled(item->isActionSupported(NET::ActionResize));
}

MoveActionImpl::MoveActionImpl(QObject *parent, TaskItem* item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item->task(), SLOT(move()));
    setText(i18n("&Move"));
    setIcon(QIcon::fromTheme("transform-move"));
    setEnabled(item->isActionSupported(NET::ActionMove));
}

CloseActionImpl::CloseActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item, SLOT(close()));
    setText(i18n("&Close"));
    setIcon(QIcon::fromTheme("window-close"));
    setEnabled(item->isActionSupported(NET::ActionClose));
}


AbstractGroupableItemAction::AbstractGroupableItemAction(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    TaskGroup *group = qobject_cast<TaskGroup *>(item);
    if (group) {
        addToTasks(group);
    } else if (TaskItem *taskItem = qobject_cast<TaskItem *>(item)) {
        m_tasks.append(taskItem->task());
    }
}

void AbstractGroupableItemAction::addToTasks(TaskGroup *group)
{
    foreach (AbstractGroupableItem * item, group->members()) {
        TaskGroup *subGroup = qobject_cast<TaskGroup *>(item);
        if (subGroup) {
            addToTasks(subGroup);
        } else if (TaskItem *taskItem = qobject_cast<TaskItem *>(item)) {
            m_tasks.append(taskItem->task());
        }
    }
}

ToCurrentDesktopActionImpl::ToCurrentDesktopActionImpl(QObject *parent, AbstractGroupableItem *item)
    : AbstractGroupableItemAction(parent, item)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotToCurrentDesktop()));
    setText(i18n("Move &To Current Desktop"));
    setEnabled(!item->isOnCurrentDesktop() && item->isActionSupported(NET::ActionChangeDesktop));
}

void ToCurrentDesktopActionImpl::slotToCurrentDesktop()
{
    const int desktop = KWindowSystem::currentDesktop();
    foreach (QWeakPointer<Task> task, m_tasks) {
        if (task) {
            task.data()->toDesktop(desktop);
        }
    }
}

ToDesktopActionImpl::ToDesktopActionImpl(QObject *parent, AbstractGroupableItem *item, int desktop)
    : AbstractGroupableItemAction(parent, item),
      m_desktop(desktop)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotToDesktop()));
    setCheckable(true);
    if (!desktop) { //to All Desktops
        setText(i18n("&All Desktops"));
        setChecked(item->isOnAllDesktops());
    } else {
        QString name = QString("&%1 %2").arg(desktop).arg(TaskManager::self()->desktopName(desktop).replace('&', "&&"));
        setText(name);
        setChecked(!item->isOnAllDesktops() && item->desktop() == desktop);
    }

}

void ToDesktopActionImpl::slotToDesktop()
{
    foreach (QWeakPointer<Task> task, m_tasks) {
        if (task) {
            task.data()->toDesktop(m_desktop);
        }
    }
}

ToNewDesktopActionImpl::ToNewDesktopActionImpl(QObject *parent, AbstractGroupableItem *item)
    : AbstractGroupableItemAction(parent, item)
{
    connect(this, SIGNAL(triggered()), this, SLOT(slotToNewDesktop()));
    setText("&New Desktop");

    m_newDesktop = TaskManager::self()->numberOfDesktops() + 1;
    if (m_newDesktop > 20) {
        setEnabled(false);
    }
}

void ToNewDesktopActionImpl::slotToNewDesktop()
{
    NETRootInfo info(QX11Info::connection(), NET::NumberOfDesktops);
    info.setNumberOfDesktops(m_newDesktop);

    foreach (QWeakPointer<Task> task, m_tasks) {
        if (task) {
            task.data()->toDesktop(m_newDesktop);
        }
    }
}

DesktopsMenu::DesktopsMenu(QWidget *parent, AbstractGroupableItem *item)
    : ToolTipMenu(parent)
{
    QActionGroup *group = new QActionGroup(this);
    setTitle(i18n("Move To &Desktop"));
    addAction(new ToCurrentDesktopActionImpl(this, item));
    QAction *action = new ToDesktopActionImpl(this, item, 0); //0 means all desktops
    addAction(action);
    group->addAction(action);
    addSeparator();
    for (int i = 1; i <= TaskManager::self()->numberOfDesktops(); i++) {
        action = new ToDesktopActionImpl(this, item, i);
        addAction(action);
        group->addAction(action);
    }
    addSeparator();
    addAction(new ToNewDesktopActionImpl(this, item));
    setEnabled(item->isActionSupported(NET::ActionChangeDesktop));
}

KeepAboveActionImpl::KeepAboveActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item, SLOT(toggleAlwaysOnTop()));
    setText(i18n("Keep &Above Others"));
    setIcon(QIcon::fromTheme("go-up"));
    setCheckable(true);
    setChecked(item->isAlwaysOnTop());
}

KeepBelowActionImpl::KeepBelowActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item, SLOT(toggleKeptBelowOthers()));
    setText(i18n("Keep &Below Others"));
    setIcon(QIcon::fromTheme("go-down"));
    setCheckable(true);
    setChecked(item->isKeptBelowOthers());
}

ViewFullscreenActionImpl::ViewFullscreenActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent)
{
    connect(this, SIGNAL(triggered()), item, SLOT(toggleFullScreen()));
    setText(i18n("&Fullscreen"));
    setIcon(QIcon::fromTheme("view-fullscreen"));
    setCheckable(true);
    setChecked(item->isFullScreen());
    setEnabled(item->isActionSupported(NET::ActionFullScreen));
}

AdvancedMenu::AdvancedMenu(QWidget *parent, AbstractGroupableItem *item, GroupManager *strategy)
    : ToolTipMenu(parent)
{
    setTitle(i18n("More Actions"));
    if (item->itemType() == TaskItemType) {
        addAction(new MoveActionImpl(this, static_cast<TaskItem *>(item)));
        addAction(new ResizeActionImpl(this, static_cast<TaskItem *>(item)));
    }
    addAction(new KeepAboveActionImpl(this, item));
    addAction(new KeepBelowActionImpl(this, item));
    addAction(new ViewFullscreenActionImpl(this, item));
    addAction(new ShadeActionImpl(this, item));

    if (strategy->taskGrouper()) {
        QList<QAction*> groupingStrategyActions = strategy->taskGrouper()->strategyActions(this, item);
        if (!groupingStrategyActions.isEmpty()) {
            addSeparator();
            foreach (QAction * action, groupingStrategyActions) {
                addAction(action);
            }
            // delete groupingStrategyActions;
        }
    }
}

LeaveGroupActionImpl::LeaveGroupActionImpl(QObject *parent, AbstractGroupableItem *item, GroupManager *strategy)
    : QAction(parent), abstractItem(item), groupingStrategy(strategy)
{
    Q_ASSERT(strategy);
    connect(this, SIGNAL(triggered()), this, SLOT(leaveGroup()));
    setText(i18n("&Leave Group"));
    setIcon(QIcon::fromTheme("window-close"));
    setEnabled(item->isGrouped());
}

void LeaveGroupActionImpl::leaveGroup()
{
    groupingStrategy->manualGroupingRequest(abstractItem, abstractItem->parentGroup()->parentGroup());
}

ToggleLauncherActionImpl::ToggleLauncherActionImpl(QObject *parent, AbstractGroupableItem *item, GroupManager *strategy)
    : QAction(parent), m_abstractItem(item), m_groupingStrategy(strategy)
{
    if (strategy->launchersLocked()) {
        setVisible(false);
        setChecked(false);
    } else {
        m_url = m_abstractItem->launcherUrl();
        // FIXME TODO: This signal delivery is queued to allow QGraphicsView to perform an ungrab
        // on the launcher item after the popup menu closes before removing the launcher destroys
        // the item, avoiding a crash. The trade-of is that it requires more care when using the
        // TaskManager::BasicMenu class, since the menu can no longer be deleted directly after
        // exec() returns. The QML 2 scene graph may be smarter than trying to perform an ungrab
        // on a destroyed item, so reinvestigate whether this is chicanery is still needed.
        connect(this, SIGNAL(triggered()), this, SLOT(toggleLauncher()), Qt::QueuedConnection);

        switch (m_abstractItem->itemType()) {
        case LauncherItemType:
            setText(i18n("Remove This Launcher"));
            break;

        case GroupItemType: {
            TaskGroup *group = static_cast<TaskGroup *>(m_abstractItem);
            foreach (AbstractGroupableItem * i, group->members()) {
                if (TaskItemType == i->itemType()) {
                    m_abstractItem = i;
                    break;
                }
            }

            if (TaskItemType != m_abstractItem->itemType()) {
                setVisible(false);
                setChecked(false);
                break;
            }

        } // fallthrough to TaskItemType below

        case TaskItemType:
            setText(i18n("&Show A Launcher When Not Running"));
            setCheckable(true);
            break;
        }
        setChecked(m_groupingStrategy->launcherExists(m_url));
    }
}

void ToggleLauncherActionImpl::toggleLauncher()
{
    if (!m_url.isValid()) {
        // No valid desktop file found, so prompt user...
        AppSelectorDialog *dlg = new AppSelectorDialog(m_abstractItem, m_groupingStrategy);
        dlg->show();
        return;
    } else if (m_groupingStrategy->launcherExists(m_url)) {
        m_groupingStrategy->removeLauncher(m_url);
    } else if (m_url.isLocalFile()) {
        QIcon icon;
        // if we don't have a desktop file, it's because it is a bare executable
        if (static_cast<TaskItem *>(m_abstractItem)->task() &&
            !KDesktopFile::isDesktopFile(m_url.toLocalFile())) {
            icon = static_cast<TaskItem *>(m_abstractItem)->task()->icon();
        }

        m_groupingStrategy->addLauncher(m_url, icon, QString(), QString(),
                                        static_cast<TaskItem *>(m_abstractItem)->task()
                                        ? static_cast<TaskItem *>(m_abstractItem)->task()->classClass()
                                        : QString());
    }
}

AppSelectorDialog::AppSelectorDialog(AbstractGroupableItem* item, GroupManager* strategy)
    : KOpenWithDialog(QList<QUrl>(), i18n("The application, to which this task is associated with, could not be determined. "
                                         "Please select the appropriate application from the list below:"), QString(), 0L),
    m_abstractItem(item),
    m_groupingStrategy(strategy)
{
    hideNoCloseOnExit();
    hideRunInTerminal();
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);
    connect(this, SIGNAL(accepted()), SLOT(launcherSelected()));
}

void AppSelectorDialog::launcherSelected()
{
    if (m_abstractItem && m_groupingStrategy) {
        KService::Ptr srv = service();
        TaskItem *taskItem = static_cast<TaskItem *>(m_abstractItem.data());
        QString wmClass = taskItem->task() ? taskItem->task()->classClass() : QString();

        if (srv && srv->isApplication() && !srv->entryPath().isEmpty()) {
            QUrl url = QUrl::fromLocalFile(srv->entryPath());

            if (url.isLocalFile() && KDesktopFile::isDesktopFile(url.toLocalFile())) {
                taskItem->setLauncherUrl(url);
                m_groupingStrategy->addLauncher(url, QIcon(), QString(), QString(), wmClass);
            }
        } else {
            QString path = text();

            if (!path.isEmpty()) {
                QUrl url = QUrl::fromLocalFile(path);
                if (url.isLocalFile()) {
                    taskItem->setLauncherUrl(url);
                    m_groupingStrategy->addLauncher(url, taskItem->icon(), taskItem->name(), wmClass, wmClass);
                }
            }
        }
    }
}

NewInstanceActionImpl::NewInstanceActionImpl(QObject *parent, AbstractGroupableItem *item)
    : QAction(parent), m_abstractItem(item)
{
    if (LauncherItemType == item->itemType()) {
        setVisible(false);
    } else {
        setIcon(QIcon::fromTheme("system-run"));
        setText(i18n("Start New Instance"));

        connect(this, SIGNAL(triggered()), this, SLOT(launchNewInstance()));
        m_url = item->launcherUrl();
        if (m_url.isEmpty()) {
            setVisible(false);
        }
    }
}

void NewInstanceActionImpl::launchNewInstance()
{
    if (!m_url.isValid()) {
        return;
    }

    new KRun(m_url, 0);
}

EditGroupActionImpl::EditGroupActionImpl(QObject *parent, TaskGroup *group, GroupManager *groupManager)
    : QAction(parent)
{
    Q_ASSERT(groupManager);
    connect(this, SIGNAL(triggered()), group, SIGNAL(groupEditRequest()));
    setText(i18n("&Edit Group"));
    //setIcon(QIcon::fromTheme("window-close"));
    bool applicable = true;
    if (groupManager->groupingStrategy()) {
        applicable = groupManager->taskGrouper()->editableGroupProperties();
    } else {
        applicable = false;
    }

    setEnabled(applicable);
    setVisible(applicable);
}

GroupingStrategyMenu::GroupingStrategyMenu(QWidget *parent, AbstractGroupableItem* item, GroupManager *strategy)
    : ToolTipMenu(parent)
{
    Q_ASSERT(item);
    Q_ASSERT(strategy);

    setTitle("Grouping strategy actions");
    if (strategy->taskGrouper()) {
        QList<QAction*> groupingStrategyActions = strategy->taskGrouper()->strategyActions(this, item);
        if (!groupingStrategyActions.empty()) {
            addSeparator();
            foreach (QAction * action, groupingStrategyActions) {
                addAction(action);
            }
        }
    }
}


BasicMenu::BasicMenu(QWidget *parent, TaskItem* item, GroupManager *strategy, QList<QAction *> visualizationActions, QList <QAction*> appActions, int maxWidth)
    : ToolTipMenu(parent)
{
    Q_ASSERT(item);
    Q_ASSERT(strategy);

    if (maxWidth) {
        setTitle(fontMetrics().elidedText(item->name(), Qt::ElideRight, maxWidth));
    } else {
        setTitle(item->name());
    }

    setIcon(item->icon());
    if (appActions.count()) {
        foreach (QAction * action, appActions) {
            if (action->isSeparator()) {
                addSeparator();
            } else {
                addAction(action);
            }
        }
        addSeparator();
    }

    if (TaskManager::self()->numberOfDesktops() > 1) {
        addMenu(new DesktopsMenu(this, item));
    }

    addAction(new MinimizeActionImpl(this, item));
    addAction(new MaximizeActionImpl(this, item));
    addAction(new NewInstanceActionImpl(this, item));
    addAction(new ToggleLauncherActionImpl(this, item, strategy));
    addMenu(new AdvancedMenu(this, item, strategy));

    foreach (QAction * action, visualizationActions) {
        addAction(action);
    }

    addSeparator();
    addAction(new CloseActionImpl(this, item));
}

BasicMenu::BasicMenu(QWidget *parent, TaskGroup* group, GroupManager *strategy, QList <QAction*> visualizationActions, QList <QAction*> appActions, int maxWidth)
    : ToolTipMenu(parent)
{
    Q_ASSERT(group);
    Q_ASSERT(strategy);

    setTitle(group->name());
    setIcon(group->icon());
    if (appActions.count()) {
        foreach (QAction * action, appActions) {
            if (action->isSeparator()) {
                addSeparator();
            } else {
                addAction(action);
            }
        }
        addSeparator();
    }

    foreach (AbstractGroupableItem * item, group->members()) {
        if (item->itemType() == GroupItemType) {
            addMenu(new BasicMenu(this, dynamic_cast<TaskGroup*>(item), strategy));
        } else {
            addMenu(new BasicMenu(this, dynamic_cast<TaskItem*>(item), strategy, QList <QAction*>(), QList <QAction*>(), maxWidth));
        }
    }
    addSeparator();

    if (TaskManager::self()->numberOfDesktops() > 1) {
        addMenu(new DesktopsMenu(this, group));
    }

    addAction(new MinimizeActionImpl(this, group));
    addAction(new MaximizeActionImpl(this, group));
    addAction(new NewInstanceActionImpl(this, group));
    addAction(new ToggleLauncherActionImpl(this, group, strategy));
    addMenu(new AdvancedMenu(this, group, strategy));
    addAction(new EditGroupActionImpl(this, group, strategy));

    foreach (QAction * action, visualizationActions) {
        addAction(action);
    }

    addSeparator();
    addAction(new CloseActionImpl(this, group));
}

BasicMenu::BasicMenu(QWidget *parent, LauncherItem* item, GroupManager *strategy, QList<QAction *> visualizationActions, QList <QAction*> appActions)
    : ToolTipMenu(parent)
{
    Q_ASSERT(item);
    Q_ASSERT(strategy);

    setTitle(item->name());
    setIcon(item->icon());
    if (appActions.count()) {
        foreach (QAction * action, appActions) {
            if (action->isSeparator()) {
                addSeparator();
            } else {
                addAction(action);
            }
        }
        addSeparator();
    }

    addAction(new ToggleLauncherActionImpl(this, item, strategy));

    if (!visualizationActions.isEmpty()) {
        addSeparator();
        foreach (QAction * action, visualizationActions) {
            addAction(action);
        }
    }
}

GroupPopupMenu::GroupPopupMenu(QWidget *parent, TaskGroup *group, GroupManager *groupManager)
    : ToolTipMenu(parent)
{
    setTitle(group->name());
    setIcon(group->icon());
    foreach (AbstractGroupableItem * item, group->members()) {
        if (!item) {
//             kDebug() << "invalid Item";
            continue;
        }

        if (item->itemType() == GroupItemType) {
            ToolTipMenu* menu = new GroupPopupMenu(this, qobject_cast<TaskGroup*>(item), groupManager);
            addMenu(menu);
        } else {
            TaskItem *taskItem = qobject_cast<TaskItem*>(item);
            if (taskItem && taskItem->task()) {
                QAction* action = new QAction(item->icon(), item->name(), this);
                connect(action, SIGNAL(triggered(bool)), taskItem->task(), SLOT(activateRaiseOrIconify()));
                addAction(action);
            }
        }
    }
}

} // TaskManager namespace

#include "taskactions.moc"
#include "moc_taskactions_p.cpp"

