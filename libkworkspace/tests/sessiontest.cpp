/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <sessionmanagement.h>

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>

int main(int argc, char **argv)
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
