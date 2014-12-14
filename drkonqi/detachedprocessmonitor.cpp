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
#include "detachedprocessmonitor.h"

#include <errno.h>
#include <signal.h>

#include <QtCore/QTimerEvent>
#include <QtCore/QDebug>

DetachedProcessMonitor::DetachedProcessMonitor(QObject *parent)
        : QObject(parent), m_pid(0)
{
}

void DetachedProcessMonitor::startMonitoring(int pid)
{
    m_pid = pid;
    startTimer(10);
}

void DetachedProcessMonitor::timerEvent(QTimerEvent *event)
{
    Q_ASSERT(m_pid != 0);
    if (::kill(m_pid, 0) < 0) {
        qDebug() << "Process" << m_pid << "finished. kill(2) returned errno:" << errno;
        killTimer(event->timerId());
        m_pid = 0;
        emit processFinished();
    }
}


