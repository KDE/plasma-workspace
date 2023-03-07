/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "playercontrol.h"

#include "playeractionjob.h"

#include <dbusproperties.h>
#include <mprisplayer.h>
#include <mprisroot.h>

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

PlayerControl::PlayerControl(PlayerContainer *container, QObject *parent)
    : Plasma5Support::Service(parent)
    , m_container(container)
{
    setObjectName(container->objectName() + QLatin1String(" controller"));
    setName(QStringLiteral("mpris2"));
    setDestination(container->objectName());

    connect(container, &Plasma5Support::DataContainer::dataUpdated, this, &PlayerControl::updateEnabledOperations);
    connect(container, &QObject::destroyed, this, &PlayerControl::containerDestroyed);
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
    setOperationEnabled(QStringLiteral("ChangeVolume"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("SetLoopStatus"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("SetRate"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("SetShuffle"), caps & PlayerContainer::CanControl);
    setOperationEnabled(QStringLiteral("GetPosition"), true);

    Q_EMIT enabledOperationsChanged();
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
    m_container = nullptr;
}

void PlayerControl::changeVolume(double delta, bool showOSD)
{
    // Not relying on property/setProperty to avoid doing blocking DBus calls

    const double volume = m_container->data().value(QStringLiteral("Volume")).toDouble();
    const double newVolume = qBound(0.0, volume + delta, qMax(volume, 1.0));

    QDBusPendingCall reply = propertiesInterface()->Set(m_container->playerInterface()->interface(), QStringLiteral("Volume"), QDBusVariant(newVolume));

    // Update the container value right away so when calling this method in quick succession
    // (mouse wheeling the tray icon) next call already gets the new value
    m_container->setData(QStringLiteral("Volume"), newVolume);

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, [this, showOSD](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();

        QDBusPendingReply<void> reply = *watcher;
        if (reply.isError()) {
            return;
        }

        if (showOSD) {
            const auto &data = m_container->data();

            QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                              QStringLiteral("/org/kde/osdService"),
                                                              QStringLiteral("org.kde.osdService"),
                                                              QStringLiteral("mediaPlayerVolumeChanged"));

            msg.setArguments({qRound(data.value(QStringLiteral("Volume")).toDouble() * 100), data.value("Identity", ""), data.value("Desktop Icon Name", "")});

            QDBusConnection::sessionBus().asyncCall(msg);
        }
    });
}

Plasma5Support::ServiceJob *PlayerControl::createJob(const QString &operation, QMap<QString, QVariant> &parameters)
{
    if (!m_container)
        return nullptr;
    return new PlayerActionJob(operation, parameters, this);
}

// vim: sw=4 sts=4 et tw=100
