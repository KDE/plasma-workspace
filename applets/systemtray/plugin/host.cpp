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

#include <Plasma/Package>
#include <Plasma/PluginLoader>

#include <QLoggingCategory>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>

#include "protocols/plasmoid/plasmoidprotocol.h"
#include "protocols/dbussystemtray/dbussystemtrayprotocol.h"
#include "tasklistmodel.h"

#define TIMEOUT 100

namespace SystemTray
{

class HostPrivate {
public:
    HostPrivate(Host *host)
        : q(host),
          rootItem(0),
          shownTasksModel(new TaskListModel(host)),
          hiddenTasksModel(new TaskListModel(host))
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
    d->setupProtocol(new SystemTray::PlasmoidProtocol(this));

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
                    if (d->showTask(task)) {
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

    qCDebug(SYSTEMTRAY) << "ST2" << task->name() << "(" << task->taskId() << ")";

    d->tasks << task;

    if (d->shownCategories.contains(task->category())) {

        qCDebug(SYSTEMTRAY) << "Adding";


        if (d->showTask(task)) {
            d->shownTasksModel->addTask(task);
        } else {
            d->hiddenTasksModel->addTask(task);
        }
    }
}

void Host::removeTask(Task *task)
{
    d->tasks.removeAll(task);
    disconnect(task, 0, this, 0);
    d->shownTasksModel->removeTask(task);
    d->hiddenTasksModel->removeTask(task);
}

void Host::slotTaskStatusChanged()
{
    Task* task = qobject_cast<Task*>(sender());

    if (task) {
        qCDebug(SYSTEMTRAY) << "ST2 emit taskStatusChanged(task);";
        taskStatusChanged(task);
    } else {
        qCDebug(SYSTEMTRAY) << "ST2 changed, but invalid cast";
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
    if (task) {
        if (d->showTask(task)) {
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
    qCDebug(SYSTEMTRAY) << "ST2 " << cats;
    return cats;
}


} // namespace

#include "host.moc"
