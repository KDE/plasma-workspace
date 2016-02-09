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
#include <QMetaObject>
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
    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> roles = roleNames();
        roles[Qt::UserRole+1] = "plugin";
        return roles;
    }
};

class HostPrivate {
public:
    HostPrivate(Host *host)
        : q(host),
          rootItem(0),
          allTasksModel(new TaskListModel(host)),
          showAllItems(false),
          availablePlasmoidsModel(Q_NULLPTR),
          plasmoidProtocol(new SystemTray::PlasmoidProtocol(host)),
          formFactor(QStringLiteral("desktop"))
    {
    }
    void setupProtocol(Protocol *protocol);
    bool showTask(Task *task) const;

    Host *q;

    QList<Task *> tasks;
    QQuickItem* rootItem;

    QSet<Task::Category> shownCategories;

    TaskListModel *allTasksModel;

    bool showAllItems;
    QStringList forcedShownItems;
    QStringList forcedHiddenItems;

    PlasmoidModel *availablePlasmoidsModel;

    SystemTray::PlasmoidProtocol *plasmoidProtocol;

    QStringList categories;
    QString formFactor;
};

Host::Host(QObject* parent) :
    QObject(parent),
    d(new HostPrivate(this))
{
    // Workaround for bug 352055
    QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
}

Host::~Host()
{
    delete d;
}

void Host::init()
{
    d->setupProtocol(new SystemTray::DBusSystemTrayProtocol(this));
    d->plasmoidProtocol->setFormFactor(d->formFactor);
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
    emit forcedShownItemsChanged();
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
    emit forcedHiddenItemsChanged();
}

bool Host::showAllItems() const
{
    return d->showAllItems;
}

void Host::setShowAllItems(bool showAllItems)
{
    if (showAllItems == d->showAllItems) {
        return;
    }

    d->showAllItems = showAllItems;
    emit showAllItemsChanged();
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
            emit shownCategoriesChanged();
        }
    } else {
        if (d->shownCategories.contains((Task::Category)cat)) {
            d->shownCategories.remove((Task::Category)cat);
            emit shownCategoriesChanged();
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
    connect(task, &Task::changedStatus, this, &Host::taskStatusChanged);

    d->tasks << task;

    d->allTasksModel->addTask(task);
}

void Host::removeTask(Task *task)
{
    d->tasks.removeAll(task);
    disconnect(task, 0, this, 0);
    d->allTasksModel->removeTask(task);
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
            if (info.property(QStringLiteral("X-Plasma-NotificationArea")) == "true") {
                applets << info;
            }
        }

        foreach (const KPluginInfo &info, applets) {
            QString name = info.name();
            KService::Ptr service = info.service();
            const QString dbusactivation = info.property(QStringLiteral("X-Plasma-DBusActivationService")).toString();

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
        if (info.isValid() && info.property(QStringLiteral("X-Plasma-NotificationArea")) == "true" &&
            info.isPluginEnabledByDefault()) {
            ret += info.pluginName();
        }
    }

    return ret;
}


bool HostPrivate::showTask(Task *task) const {
    return task->shown() && task->status() != SystemTray::Task::Passive &&
        task->status() != SystemTray::Task::HiddenStatus;
}

void HostPrivate::setupProtocol(Protocol *protocol)
{
    QObject::connect(protocol, SIGNAL(taskCreated(SystemTray::Task*)), q, SLOT(addTask(SystemTray::Task*)));
    protocol->init();
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

QString Host::formFactor() const
{
    return d->formFactor;
}

void Host::setFormFactor(const QString &formfactor)
{
    if (d->formFactor != formfactor) {
        d->formFactor = formfactor;
        d->plasmoidProtocol->setFormFactor(d->formFactor);
        emit formFactorChanged();
    }

}

} // namespace


