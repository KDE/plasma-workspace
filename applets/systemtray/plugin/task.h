/***************************************************************************
 *   task.h                                                                *
 *                                                                         *
 *   Copyright (C) 2008 Jason Stubbs <jasonbstubbs@gmail.com>              *
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

#ifndef SYSTEMTRAYTASK_H
#define SYSTEMTRAYTASK_H

#include <QtCore/QObject>

#include <QtGui/QIcon>
#include <QQuickItem>

#include "debug.h"

namespace SystemTray
{

/**
 * @short System tray task base class
 *
 * To support a new system tray protocol, Protocol and this class should
 * be subclassed.
 **/
class Task : public QObject
{
    Q_OBJECT

    Q_ENUMS(Status)
    Q_ENUMS(Category)
    Q_ENUMS(TaskType)

    Q_PROPERTY(TaskType type READ type CONSTANT)
    Q_PROPERTY(QString taskId READ taskId CONSTANT)
    Q_PROPERTY(QQuickItem* taskItem READ taskItem NOTIFY taskItemChanged)
    Q_PROPERTY(QQuickItem* taskItemExpanded READ taskItem NOTIFY taskItemExpandedChanged)
    Q_PROPERTY(Status status READ status NOTIFY changedStatus)
    Q_PROPERTY(QString name READ name NOTIFY changedName)
    Q_PROPERTY(Category category READ category NOTIFY changedCategory)
    Q_PROPERTY(bool expanded READ expanded WRITE setExpanded NOTIFY expandedChanged)

public:
    enum Status {
        UnknownStatus = 0,
        Passive = 1,
        Active = 2,
        NeedsAttention = 3
    };

    enum Category {
        UnknownCategory = 0,
        ApplicationStatus = 1,
        Communications = 2,
        SystemServices = 3,
        Hardware = 4
    };

    /**
     * Derived classes should provide its type. We assume that number of different types of tasks is
     * a limited value. So, it's easier to provide constants for each type of tasks than always
     * try to cast classes. Moreover, these contants are used in QML code.
     */
    enum TaskType {
        TypePlasmoid,
        TypeX11Task,
        TypeStatusItem,
        TypeUserDefined
    };

    Task(QObject *parent = 0);

    virtual ~Task();

    /**
     * Returns whether this task can be embeddable
     *
     * Depending on the protocol, there may be circumstances under which
     * a new widget can not be created. isEmbeddable() will return false
     * under these circumstances.
     **/
    virtual bool isEmbeddable() const = 0;

    /**
     * Returns whether this task is represented as widget or it provides only information (icon, name, state, etc)
     * @return true if task is represented as widget.
     */
    virtual bool isWidget() const = 0;

    /**
     * Returns the name of this task that should be presented to the user
     **/
    QString name() const;

    void setName(QString name);

    bool shown() const;

    void setShown(bool show);

    /**
     * Returns a unique identifier for this task
     *
     * The identifier is valid between restarts and so is safe to save
     **/
    virtual QString taskId() const = 0;

    virtual QQuickItem* taskItem() const;
    virtual QQuickItem* taskItemExpanded() const;

    /**
     * Returns an icon that can be associated with this task
     *
     * The icon returned is not necessarily the same icon that appears
     * in the tray icon itself.
     **/
    virtual QIcon icon() const = 0;

    /**
     * @return true if this task is current being used, e.g. it has created
     * widgets for one or more hosts
     */
//     bool isUsed() const;

    /**
     * Sets the category of the task, UnknownCategory by default
     * @arg category the category for this task
     */
    void setCategory(Category category);

    /**
     * @return the category of this task
     */
    Category category() const;

    /**
     * Sets the status of the task, UnknownStatus by default.
     * @arg status the status for this task
     */
    void setStatus(Status status);

    /**
     * @return the status for this task
     */
    Status status() const;

    virtual bool expanded() const;
    virtual void setExpanded(bool expanded);

    /**
     * This function must always return type of task (an integer value). This value must always be
     * the same for each call of function.
     * @return a type of task.
     */
    virtual TaskType type() const = 0;

    Q_INVOKABLE QPointF popupPosition(QQuickItem *visualParent, int x, int y);

Q_SIGNALS:
    /**
     * Emitted when something about the task has changed
     **/
    //TODO: this should also state _what_ was changed so we can react more
    //      precisely (and therefore with greater efficiency)
    void changed(SystemTray::Task *task);
    void shownChanged();

    void taskItemChanged();
    void taskItemExpandedChanged();
    void expandedChanged(bool expanded);

    /**
     * Special signal for changed status
     */
    void changedStatus();

    // if a category of task has been changed
    void changedCategory();

    // If a name of task has been changed
    void changedName();

    /// if visibility preference of task is changed
    void changedVisibilityPreference();

    /**
     * Emitted when the task is about to be destroyed
     **/
    void destroyed(SystemTray::Task *task);

private Q_SLOTS:
    void emitChanged();

private:
    class Private;
    Private* const d;
};

}


#endif
