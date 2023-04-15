/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include "playercontainer.h"

#include <Plasma/Service>
#include <QDBusObjectPath>

class OrgFreedesktopDBusPropertiesInterface;
class OrgMprisMediaPlayer2Interface;
class OrgMprisMediaPlayer2PlayerInterface;

class PlayerControl : public Plasma::Service
{
    Q_OBJECT

public:
    PlayerControl(PlayerContainer *container, QObject *parent);

    OrgMprisMediaPlayer2Interface *rootInterface() const
    {
        return m_container->rootInterface();
    }
    OrgMprisMediaPlayer2PlayerInterface *playerInterface() const
    {
        return m_container->playerInterface();
    }
    OrgFreedesktopDBusPropertiesInterface *propertiesInterface() const
    {
        return m_container->propertiesInterface();
    }
    void updatePosition() const
    {
        m_container->updatePosition();
    }
    PlayerContainer::Caps capabilities() const
    {
        return m_container->capabilities();
    }
    const QMap<QString, QVariant> /*DataEngine::Data*/ rawData() const
    {
        return m_container->data();
    }

    QDBusObjectPath trackId() const;

    Plasma::ServiceJob *createJob(const QString &operation, QMap<QString, QVariant> &parameters) override;

    void changeVolume(double delta, bool showOSD);

    PlayerContainer *container() const
    {
        return m_container;
    }

Q_SIGNALS:
    void enabledOperationsChanged();

private Q_SLOTS:
    void updateEnabledOperations();
    void containerDestroyed();

private:
    PlayerContainer *m_container;
};
