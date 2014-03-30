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
#include <zlib.h>

#include "protocols/plasmoid/plasmoidprotocol.h"
#include "protocols/dbussystemtray/dbussystemtrayprotocol.h"

#define TIMEOUT 100

namespace SystemTray
{


bool taskLessThan(const Task *lhs, const Task *rhs)
{
    /* Sorting of systemtray icons
     *
     * We sort (and thus group) in the following order, from high to low priority
     * - Notifications always comes first
     * - Category
     * - Name
     */

    const QString _not = QStringLiteral("org.kde.plasma.notifications");
    if (lhs->taskId() == _not) {
        return true;
    }
    if (rhs->taskId() == _not) {
        return false;
    }

    if (lhs->category() != rhs->category()) {
        QMap<Task::Category, int> weights;
        weights.insert(Task::Communications, 0);
        weights.insert(Task::SystemServices, 1);
        weights.insert(Task::Hardware, 2);
        weights.insert(Task::ApplicationStatus, 3);
        weights.insert(Task::UnknownCategory, 4);

        return weights.value(lhs->category()) < weights.value(rhs->category());
    }

    return lhs->name() < rhs->name();
}

class HostPrivate {
public:
    HostPrivate(Host *host)
        : q(host),
          rootItem(0)
    {
    }
    void setupProtocol(Protocol *protocol);
    bool showTask(Task *task);


    Host *q;

    QList<Task *> tasks;
    QQuickItem* rootItem;

    // Keep references to the list to avoid full refreshes
    //QList<SystemTray::Task*> tasks;
    QList<SystemTray::Task*> shownTasks;
    QList<SystemTray::Task*> hiddenTasks;
    QQmlListProperty<SystemTray::Task> shownTasksDeclarative;
    QQmlListProperty<SystemTray::Task> hiddenTasksDeclarative;
    //QQmlListProperty<SystemTray::Task> tasksDeclarative;

    QStringList categories;

    QTimer compressionTimer;
};

Host::Host(QObject* parent) :
    QObject(parent),
    d(new HostPrivate(this))
{
//        connect(m_manager, &Manager::tasksChanged, this, &Host::tasksChanged);

    QTimer::singleShot(2000, this, SLOT(init())); // FIXME: remove timer
    //init();
    connect(&d->compressionTimer, &QTimer::timeout, this, &Host::compressionTimeout);
}

Host::~Host()
{
    delete d;
}

void Host::init()
{
    initTasks();

    d->setupProtocol(new SystemTray::DBusSystemTrayProtocol(this));
    d->setupProtocol(new SystemTray::PlasmoidProtocol(this));


    emit categoriesChanged();
}

void Host::compressionTimeout()
{
    qCDebug(SYSTEMTRAY) << "ST2 tasksChanged";
    emit tasksChanged();
    d->compressionTimer.stop();

}


void Host::initTasks()
{
    QList<SystemTray::Task*> allTasks = tasks();
    foreach (SystemTray::Task *task, allTasks) {
        if (d->showTask(task)) {
            d->shownTasks.append(task);
        } else {
            d->hiddenTasks.append(task);
        }
    }

    qSort(d->shownTasks.begin(), d->shownTasks.end(), taskLessThan);
    qSort(d->hiddenTasks.begin(), d->hiddenTasks.end(), taskLessThan);

    QQmlListProperty<SystemTray::Task> _shown(this, d->shownTasks);
    d->shownTasksDeclarative = _shown;

    QQmlListProperty<SystemTray::Task> _hidden(this, d->hiddenTasks);
    d->hiddenTasksDeclarative = _hidden;
    qCDebug(SYSTEMTRAY) << "ST2 init starting timer" << TIMEOUT;
    d->compressionTimer.start(TIMEOUT);
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

QList<Task*> Host::tasks() const
{
    return d->tasks;
}

void Host::addTask(Task *task)
{
    connect(task, SIGNAL(destroyed(SystemTray::Task*)), this, SLOT(removeTask(SystemTray::Task*)));
    connect(task, SIGNAL(changedStatus()), this, SLOT(slotTaskStatusChanged()));

    qCDebug(SYSTEMTRAY) << "ST2" << task->name() << "(" << task->taskId() << ")";

    d->tasks.append(task);
    if (d->showTask(task)) {
        d->shownTasks.append(task);
        qSort(d->shownTasks.begin(), d->shownTasks.end(), taskLessThan);
    } else {
        d->hiddenTasks.append(task);
        qSort(d->hiddenTasks.begin(), d->hiddenTasks.end(), taskLessThan);
    }
    d->compressionTimer.start(TIMEOUT);
    emit tasksChanged();
}

void Host::removeTask(Task *task)
{
    d->tasks.removeAll(task);
    disconnect(task, 0, this, 0);
    if (d->showTask(task)) {
        d->shownTasks.removeAll(task);
    } else {
        d->hiddenTasks.removeAll(task);
    }
    // No compression here, as we delete the pointer to the task
    // object behind the list's back otherwise
    emit tasksChanged();
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

QQmlListProperty<SystemTray::Task> Host::hiddenTasks()
{
    return d->hiddenTasksDeclarative;
}

QQmlListProperty< Task > Host::shownTasks()
{
    return d->shownTasksDeclarative;

}

bool HostPrivate::showTask(Task *task) {
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
        if (d->shownTasks.contains(task)) {
            if (!d->showTask(task)) {
                qCDebug(SYSTEMTRAY) << "ST2 Migrating shown -> hidden" << task->name();
                d->shownTasks.removeAll(task);
                d->hiddenTasks.append(task);
                qSort(d->hiddenTasks.begin(), d->hiddenTasks.end(), taskLessThan);
                d->compressionTimer.start(TIMEOUT);
            }
        } else if (d->hiddenTasks.contains(task)) {
            if (d->showTask(task)) {
                qCDebug(SYSTEMTRAY) << "ST2 Migrating hidden -> shown" << task->name();
                d->hiddenTasks.removeAll(task);
                d->shownTasks.append(task);
                qSort(d->shownTasks.begin(), d->shownTasks.end(), taskLessThan);
                d->compressionTimer.start(TIMEOUT);
            }
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
