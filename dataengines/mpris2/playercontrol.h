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

#ifndef PLAYERCONTROL_H
#define PLAYERCONTROL_H

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
    PlayerControl(PlayerContainer* container, QObject* parent);

    OrgMprisMediaPlayer2Interface* rootInterface() const
        { return m_container->rootInterface(); }
    OrgMprisMediaPlayer2PlayerInterface* playerInterface() const
        { return m_container->playerInterface(); }
    OrgFreedesktopDBusPropertiesInterface* propertiesInterface() const
        { return m_container->propertiesInterface(); }
    void updatePosition() const
        { m_container->updatePosition(); }
    PlayerContainer::Caps capabilities() const
        { return m_container->capabilities(); }
    const QMap<QString, QVariant> /*DataEngine::Data*/ rawData() const
        { return m_container->data(); }

    QDBusObjectPath trackId() const;

    Plasma::ServiceJob* createJob(const QString& operation,
                                  QMap<QString,QVariant>& parameters) override;

    void changeVolume(double delta, bool showOSD);

Q_SIGNALS:
    void enabledOperationsChanged();

private Q_SLOTS:
    void updateEnabledOperations();
    void containerDestroyed();

private:
    PlayerContainer *m_container;
};

#endif // PLAYERCONTROL_H
