/*
    Copyright (C) 2009  George Kiagiadakis <gkiagia@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DETACHEDPROCESSMONITOR_H
#define DETACHEDPROCESSMONITOR_H

#include <QtCore/QObject>

class DetachedProcessMonitor : public QObject
{
    Q_OBJECT
public:
    explicit DetachedProcessMonitor(QObject *parent = 0);
    void startMonitoring(int pid);

Q_SIGNALS:
    void processFinished();

protected:
    virtual void timerEvent(QTimerEvent *);

private:
    int m_pid;
};

#endif
