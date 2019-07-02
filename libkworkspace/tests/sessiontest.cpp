/*
   Copyright (C) 2019 David Edmundson <davidedmundson@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the Lesser GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the Lesser GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include <sessionmanagement.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QDebug>

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);
    auto session = new SessionManagement(&app);

    QEventLoop e;
    if (session->state() == SessionManagement::State::Loading) {
        QObject::connect(session, &SessionManagement::stateChanged, &e, &QEventLoop::quit);
        e.exec();
    }

    qDebug() << session->state();
    qDebug() << "canShutdown" << session->canShutdown();
    qDebug() << "canReboot" << session->canReboot();
    qDebug() << "canLogout" << session->canLogout();
    qDebug() << "canSuspend" << session->canSuspend();
    qDebug() << "canHibernate" << session->canHibernate();
    qDebug() << "canSwitchUser" << session->canSwitchUser();
    qDebug() << "canLock" << session->canLock();
    qDebug() << "canSwitchUser" << session->canSwitchUser();
}
