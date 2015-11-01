/*
 * Copyright 2008  Alex Merry <alex.merry@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include "playercontrol.h"

#include "playeractionjob.h"
#include "playercontainer.h"

#include <dbusproperties.h>
#include <mprisplayer.h>
#include <mprisroot.h>

#include <QDBusConnection>

PlayerControl::PlayerControl(PlayerContainer* container, QObject* parent)
    : Plasma::Service(parent)
    , m_container(container)
{
    setObjectName(container->objectName() + QLatin1String(" controller"));
    setName(QStringLiteral("mpris2"));
    setDestination(container->objectName());

    connect(container, &Plasma::DataContainer::dataUpdated,
            this,      &PlayerControl::updateEnabledOperations);
    connect(container, &QObject::destroyed,
            this,      &PlayerControl::containerDestroyed);
    updateEnabledOperations();
}

void PlayerControl::updateEnabledOperations()
{
    PlayerContainer::Caps caps = PlayerContainer::NoCaps;
    if (m_container)
        caps = m_container->capabilities();

    setOperationEnabled(QStringLiteral("Quit"), caps & PlayerContainer::CanQuit);
    setOperationEnabled(QStringLiteral("Raise"), caps & PlayerContainer::CanRaise);
    setOperationEnabled(QStringLiteral("SetFullscreen"), caps & PlayerContainer::CanSetFullscreen);

    setOperationEnabled(QStringLiteral("Play"), caps & PlayerContainer::CanPlay);
    setOperationEnabled(QStringLiteral("Pause"), caps & PlayerContainer::CanPause);
    setOperationEnabled(QStringLiteral("PlayPause"), caps & (PlayerContainer::CanPlay | PlayerContainer::CanPause));
    setOperationEnabled(QStringLiteral("Stop"), caps & PlayerContainer::CanStop);
    setOperationEnabled(QStringLiteral("Next"), caps & PlayerContainer::CanGoNext);
    setOperationEnabled(QStringLiteral("Previous"), caps & PlayerContainer::CanGoPrevious);
    setOperationEnabled(QStringLiteral("Seek"), caps & PlayerContainer::CanSeek);
    setOperationEnabled(QStringLiteral("SetPosition"), caps & PlayerContainer::CanSeek);
    setOperationEnabled(QStringLiteral("OpenUri"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("SetVolume"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("SetLoopStatus"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("SetRate"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("SetShuffle"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("GetPosition"), true);

    emit enabledOperationsChanged();
}

QDBusObjectPath PlayerControl::trackId() const
{
    QVariant mprisTrackId = m_container->data().value(QStringLiteral("Metadata")).toMap().value(QStringLiteral("mpris:trackid"));
    if (mprisTrackId.canConvert<QDBusObjectPath>()) {
        return mprisTrackId.value<QDBusObjectPath>();
    }
    QString mprisTrackIdString = mprisTrackId.toString();
    if (!mprisTrackIdString.isEmpty()) {
        return QDBusObjectPath(mprisTrackIdString);
    }
    return QDBusObjectPath();
}

void PlayerControl::containerDestroyed()
{
    m_container = 0;
}

Plasma::ServiceJob* PlayerControl::createJob(const QString& operation,
                                             QMap<QString,QVariant>& parameters)
{
    if (!m_container)
        return 0;
    return new PlayerActionJob(operation, parameters, this);
}

// vim: sw=4 sts=4 et tw=100
