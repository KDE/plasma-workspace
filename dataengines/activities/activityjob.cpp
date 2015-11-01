/*
 * Copyright 2009 Chani Armitage <chani@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "activityjob.h"

#include <kactivities/controller.h>
#include <QDebug>
#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>

ActivityJob::ActivityJob(KActivities::Controller *controller, const QString &id, const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent) :
    ServiceJob(parent->objectName(), operation, parameters, parent),
    m_activityController(controller),
    m_id(id)
{
}

ActivityJob::~ActivityJob()
{
}

void ActivityJob::start()
{
    const QString operation = operationName();
    if (operation == QLatin1String("add")) {
        //I wonder how well plasma will handle this...
        QString name = parameters()[QStringLiteral("Name")].toString();
        if (name.isEmpty()) {
            name = i18n("unnamed");
        }
        const QString activityId = m_activityController->addActivity(name);
        setResult(activityId);
        return;
    }
    if (operation == QLatin1String("remove")) {
        QString id = parameters()[QStringLiteral("Id")].toString();
        m_activityController->removeActivity(id);
        setResult(true);
        return;
    }

    //m_id is needed for the rest
    if (m_id.isEmpty()) {
        setResult(false);
        return;
    }
    if (operation == QLatin1String("setCurrent")) {
        m_activityController->setCurrentActivity(m_id);
        setResult(true);
        return;
    }
    if (operation == QLatin1String("stop")) {
        m_activityController->stopActivity(m_id);
        setResult(true);
        return;
    }
    if (operation == QLatin1String("start")) {
        m_activityController->startActivity(m_id);
        setResult(true);
        return;
    }
    if (operation == QLatin1String("setName")) {
        m_activityController->setActivityName(m_id, parameters()[QStringLiteral("Name")].toString());
        setResult(true);
        return;
    }
    if (operation == QLatin1String("setIcon")) {
        m_activityController->setActivityIcon(m_id, parameters()[QStringLiteral("Icon")].toString());
        setResult(true);
        return;
    }
    if (operation == QLatin1String("toggleActivityManager")) {
        QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                          QStringLiteral("/PlasmaShell"),
                                                          QStringLiteral("org.kde.PlasmaShell"),
                                                          QStringLiteral("toggleActivityManager"));
        QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
        setResult(true);
        return;
    }
    setResult(false);
}


