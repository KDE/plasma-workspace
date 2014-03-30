/***************************************************************************
 *   taskprotocol.h                                                        *
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

#ifndef SYSTEMTRAYPROTOCOL_H
#define SYSTEMTRAYPROTOCOL_H

#include <QtCore/QObject>

namespace SystemTray
{
    class Job;
    class Notification;
    class Task;
}


namespace SystemTray
{

/**
 * @short System tray protocol base class
 *
 * To support a new system tray protocol, this class and Task should be
 * subclassed and the subclass of this class registered with the global
 * Manager. The Protocol subclass should emit taskCreated() for each new
 * task created.
 **/
class Protocol : public QObject
{
    Q_OBJECT
public:
    explicit Protocol(QObject *parent);

    virtual void init() = 0;

Q_SIGNALS:
    /**
     * Signals that a new task has been created
     **/
    void taskCreated(SystemTray::Task *task);

    /**
     * Signals that a new notification has been created
     **/
    void jobCreated(SystemTray::Job *job);

    /**
     * Signals that a new notification has been created
     **/
    void notificationCreated(SystemTray::Notification *notification);
};

}


#endif
