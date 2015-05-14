/***************************************************************************
 *                                                                         *
 *   Copyright 2013 Sebastian Kügler <sebas@kde.org>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/


#include "host.h"
#include "task.h"
#include "debug.h"
#include "protocol.h"

#include <klocalizedstring.h>
#include <KActionCollection>

#include <Plasma/Package>
#include <Plasma/PluginLoader>

#include <QLoggingCategory>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>
#include <QStandardItemModel>
#include <QMenu>

#include "protocols/plasmoid/plasmoidprotocol.h"
#include "protocols/plasmoid/plasmoidtask.h"
#include "protocols/dbussystemtray/dbussystemtrayprotocol.h"
#include "tasklistmodel.h"

#define TIMEOUT 100

namespace SystemTray
{

class PlasmoidModel: public QStandardItemModel
{
public:
    PlasmoidModel(QObject *parent = 0)
        : QStandardItemModel(parent)
    {
        QHash<int, QByteArray> roles = roleNames();
        roles[Qt::UserRole+1] = "plugin";
        setRoleNames(roles);
    }
};

class HostPrivate {
public:
    HostPrivate(Host *host)
        : q(host),
          rootItem(0),
          shownTasksModel(new TaskListModel(host)),
          hiddenTasksModel(new TaskListModel(host)),
          allTasksModel(new TaskListModel(host)),
          availablePlasmoidsModel(Q_NULLPTR),
          plasmoidProtocol(new SystemTray::PlasmoidProtocol(host))
    {
    }
    void setupProtocol(Protocol *protocol);
    bool showTask(Task *task) const;

    Host *q;

    QList<Task *> tasks;
    QQuickItem* rootItem;

    QSet<Task::Category> shownCategories;

    TaskListModel *shownTasksModel;
    TaskListModel *hiddenTasksModel;
    TaskListModel *allTasksModel;

    QStringList forcedShownItems;
    QStringList forcedHiddenItems;

    PlasmoidModel *availablePlasmoidsModel;

    SystemTray::PlasmoidProtocol *plasmoidProtocol;

    QStringList categories;
};

Host::Host(QObject* parent) :
    QObject(parent),
    d(new HostPrivate(this))
{
    QTimer::singleShot(0, this, SLOT(init()));
}

Host::~Host()
{
    delete d;
}

void Host::init()
{
    d->setupProtocol(new SystemTray::DBusSystemTrayProtocol(this));
    d->setupProtocol(d->plasmoidProtocol);

    initTasks();

    emit categoriesChanged();
}

void Host::initTasks()
{
    QList<SystemTray::Task*> allTasks = tasks();
    foreach (SystemTray::Task *task, allTasks) {
        addTask(task);
    }
}

QQuickItem* Host::rootItem()
{
    return d->rootItem;
}

QStringList Host::plasmoidsAllowed() const
{
    if (d->plasmoidProtocol) {
        return d->plasmoidProtocol->allowedPlugins();
    } else {
        return QStringList();
    }
}

void Host::setPlasmoidsAllowed(const QStringList &plasmoids)
{
    if (d->plasmoidProtocol) {
        d->plasmoidProtocol->setAllowedPlugins(plasmoids);
        emit plasmoidsAllowedChanged();
    }
}

QStringList Host::forcedShownItems() const
{
    return d->forcedShownItems;
}

void Host::setForcedShownItems(const QStringList &items)
{
    if (items == d->forcedShownItems) {
        return;
    }

    d->forcedShownItems = items;

    foreach (Task *task, d->tasks) {
        if (d->shownCategories.contains(task->category())) {
            if (!d->forcedHiddenItems.contains(task->taskId()) &&
                (d->forcedShownItems.contains(task->taskId()) || d->showTask(task))) {
                d->hiddenTasksModel->removeTask(task);
                d->shownTasksModel->addTask(task);
            } else {
                d->shownTasksModel->removeTask(task);
                d->hiddenTasksModel->addTask(task);
            }
        }
    }
}


QStringList Host::forcedHiddenItems() const
{
    return d->forcedHiddenItems;
}

void Host::setForcedHiddenItems(const QStringList &items)
{
    if (items == d->forcedHiddenItems) {
        return;
    }

    d->forcedHiddenItems = items;

    foreach (Task *task, d->tasks) {
        if (d->shownCategories.contains(task->category())) {
            if (!d->forcedHiddenItems.contains(task->taskId()) &&
                (d->forcedShownItems.contains(task->taskId()) || d->showTask(task))) {
                d->hiddenTasksModel->removeTask(task);
                d->shownTasksModel->addTask(task);
            } else {
                d->shownTasksModel->removeTask(task);
                d->hiddenTasksModel->addTask(task);
            }
        }
    }
}

void Host::setRootItem(QQuickItem* item)
{
    if (d->rootItem == item) {
        return;
    }

    d->rootItem = item;
    emit rootItemChanged();
}

bool Host::isCategoryShown(int cat) const
{
    return d->shownCategories.contains((Task::Category)cat);
}

void Host::setCategoryShown(int cat, bool shown)
{
    if (shown) {
        if (!d->shownCategories.contains((Task::Category)cat)) {
            d->shownCategories.insert((Task::Category)cat);
            foreach (Task *task, d->tasks) {
                if (task->category() == cat) {
                    if (!d->forcedHiddenItems.contains(task->taskId()) &&
                        (d->forcedShownItems.contains(task->taskId()) || d->showTask(task))) {
                        d->shownTasksModel->addTask(task);
                    } else {
                        d->hiddenTasksModel->addTask(task);
                    }
                }
            }
        }
    } else {
        if (d->shownCategories.contains((Task::Category)cat)) {
            d->shownCategories.remove((Task::Category)cat);
            foreach (Task *task, d->tasks) {
                if (task->category() == cat) {
                    d->shownTasksModel->removeTask(task);
                    d->hiddenTasksModel->removeTask(task);
                }
            }
        }
    }
}

QList<Task*> Host::tasks() const
{
    return d->tasks;
}

void Host::addTask(Task *task)
{
    connect(task, SIGNAL(destroyed(SystemTray::Task*)), this, SLOT(removeTask(SystemTray::Task*)));
    connect(task, SIGNAL(changedStatus()), this, SLOT(slotTaskStatusChanged()));

    d->tasks << task;

    if (d->shownCategories.contains(task->category())) {
        if (!d->forcedHiddenItems.contains(task->taskId()) &&
            (d->forcedShownItems.contains(task->taskId()) || d->showTask(task))) {
            d->shownTasksModel->addTask(task);
        } else {
            d->hiddenTasksModel->addTask(task);
        }
    }

    d->allTasksModel->addTask(task);
}

void Host::removeTask(Task *task)
{
    d->tasks.removeAll(task);
    disconnect(task, 0, this, 0);
    d->shownTasksModel->removeTask(task);
    d->hiddenTasksModel->removeTask(task);
    d->allTasksModel->removeTask(task);
}

void Host::slotTaskStatusChanged()
{
    Task* task = qobject_cast<Task*>(sender());

    if (task) {
        taskStatusChanged(task);
    }
}

QAbstractItemModel* Host::hiddenTasks()
{
    return d->hiddenTasksModel;
}

QAbstractItemModel* Host::shownTasks()
{
    return d->shownTasksModel;
}

QAbstractItemModel* Host::allTasks()
{
    return d->allTasksModel;
}

QAbstractItemModel* Host::availablePlasmoids()
{
    if (!d->availablePlasmoidsModel) {
        d->availablePlasmoidsModel = new PlasmoidModel(this);

        //Filter X-Plasma-NotificationArea
        KPluginInfo::List applets;
        for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
            if (info.property("X-Plasma-NotificationArea") == "true") {
                applets << info;
            }
        }

        foreach (const KPluginInfo &info, applets) {
            QString name = info.name();
            KService::Ptr service = info.service();
            const QString dbusactivation = info.property("X-Plasma-DBusActivationService").toString();

            if (!dbusactivation.isEmpty()) {
                name += i18n(" (Automatic load)");
            }
            QStandardItem *item = new QStandardItem(QIcon::fromTheme(info.icon()), name);
            item->setData(info.pluginName());
            d->availablePlasmoidsModel->appendRow(item);
        }
    }
    return d->availablePlasmoidsModel;
}

QStringList Host::defaultPlasmoids() const
{
    QStringList ret;
    for (auto info : Plasma::PluginLoader::self()->listAppletInfo(QString())) {
        if (info.isValid() && info.property("X-Plasma-NotificationArea") == "true" &&
            info.isPluginEnabledByDefault()) {
            ret += info.pluginName();
        }
    }

    return ret;
}


bool HostPrivate::showTask(Task *task) const {
    return task->shown() && task->status() != SystemTray::Task::Passive;
}

void HostPrivate::setupProtocol(Protocol *protocol)
{
    QObject::connect(protocol, SIGNAL(taskCreated(SystemTray::Task*)), q, SLOT(addTask(SystemTray::Task*)));
    protocol->init();
}

void Host::taskStatusChanged(SystemTray::Task *task)
{
    if (!d->shownCategories.contains(task->category())) {
        return;
    }

    if (task) {
        if (!d->forcedHiddenItems.contains(task->taskId()) &&
            (d->forcedShownItems.contains(task->taskId()) || d->showTask(task))) {
            d->hiddenTasksModel->removeTask(task);
            d->shownTasksModel->addTask(task);
        } else {
            d->hiddenTasksModel->addTask(task);
            d->shownTasksModel->removeTask(task);
        }
    }
}

QStringList Host::categories() const
{
    QList<SystemTray::Task*> allTasks = tasks();
    QStringList cats;
    QList<SystemTray::Task::Category> cnt;
    foreach (SystemTray::Task *task, allTasks) {
        const SystemTray::Task::Category c = task->category();
        if (cnt.contains(c)) {
            continue;
        }
        cnt.append(c);

        if (c == SystemTray::Task::UnknownCategory) {
            cats.append(i18n("Unknown Category"));
        } else if (c == SystemTray::Task::ApplicationStatus) {
            cats.append(i18n("Application Status"));
        } else if (c == SystemTray::Task::Communications) {
            cats.append(i18n("Communications"));
        } else if (c == SystemTray::Task::SystemServices) {
            cats.append(i18n("System Services"));
        } else if (c == SystemTray::Task::Hardware) {
            cats.append(i18n("Hardware"));
        }
    }
    return cats;
}

void Host::showMenu(int x, int y, QObject *task)
{
    SystemTray::PlasmoidTask *pt = qobject_cast<SystemTray::PlasmoidTask *>(task);
    if (!pt) {
        return;
    }

    Plasma::Applet *a = pt->applet();
    if (!a) {
        return;
    }

    QMenu desktopMenu;
    for (auto action : a->contextualActions()) {
        if (action) {
            desktopMenu.addAction(action);
        }
    }
    QAction *runAssociatedApplication = a->actions()->action("run associated application");
    if (runAssociatedApplication && runAssociatedApplication->isEnabled()) {
        desktopMenu.addAction(runAssociatedApplication);
    }

    QAction *configureApplet = a->actions()->action("configure");
    if (configureApplet && configureApplet->isEnabled()) {
        desktopMenu.addAction(configureApplet);
    }

    Plasma::Applet *systrayApplet = qobject_cast<Plasma::Applet *>(a->containment()->parent());
    if (systrayApplet) {
        QMenu *trayMenu = new QMenu(i18nc("%1 is the name of the applet", "%1 Options", systrayApplet->title()), &desktopMenu);
        trayMenu->addAction(systrayApplet->actions()->action("configure"));
        trayMenu->addAction(systrayApplet->actions()->action("remove"));
        desktopMenu.addMenu(trayMenu);

        Plasma::Containment *c = systrayApplet->containment();
        if (c) {
            QMenu *containmentMenu = new QMenu(i18nc("%1 is the name of the containment", "%1 Options", c->title()), &desktopMenu);
            containmentMenu->addAction(c->actions()->action("configure"));
            containmentMenu->addAction(c->actions()->action("remove"));
            desktopMenu.addMenu(containmentMenu);
        }
    }

    if (!desktopMenu.isEmpty()) {
        desktopMenu.exec(QPoint(x, y));
    }
}

} // namespace


