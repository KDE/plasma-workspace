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


#ifndef SYSTEMTRAYMANAGER__H
#define SYSTEMTRAYMANAGER__H

#include <kdeclarative/qmlobject.h>

#include <QQmlListProperty>
#include <QObject>
#include <qtextcodec.h>

class QQuickItem;

namespace SystemTray {
    class Manager;
    class Task;
}

namespace SystemTray {

class HostPrivate;

class Host : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QQmlListProperty<SystemTray::Task> hiddenTasks READ hiddenTasks NOTIFY tasksChanged)
    Q_PROPERTY(QQmlListProperty<SystemTray::Task> shownTasks READ shownTasks NOTIFY tasksChanged)

    Q_PROPERTY(QStringList categories READ categories NOTIFY categoriesChanged)

    Q_PROPERTY(QQuickItem* rootItem READ rootItem WRITE setRootItem NOTIFY rootItemChanged)
    //Q_PROPERTY(QQuickItem* rootItem WRITE setRootItem)

public:
    Host(QObject* parent = 0);
    virtual ~Host();

    /**
     * @return a list of all known Task instances
     **/
    QList<Task*> tasks() const;

    void setRootItem(QQuickItem* rootItem);
    QQuickItem* rootItem();

public Q_SLOTS:
    void init();
    QQmlListProperty<SystemTray::Task> hiddenTasks();
    QQmlListProperty<SystemTray::Task> shownTasks();
    QStringList categories() const;


Q_SIGNALS:
    void tasksChanged();
    void categoriesChanged();
    void rootItemChanged();

private Q_SLOTS:
    void addTask(SystemTray::Task *task);
    void removeTask(SystemTray::Task *task);
    void slotTaskStatusChanged();
    void taskStatusChanged(SystemTray::Task *task);
    void compressionTimeout();

private:
    void initTasks();
    HostPrivate* d;

};

} // namespace
#endif // SYSTEMTRAYMANAGER__H
