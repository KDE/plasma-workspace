/*
 * Copyright 2021 Kai Uwe Broulik <kde@broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "soundscheme.h"

#include <QString>

#include "debug.h"
#include "eventsound.h"
#include "server.h"
#include "server_p.h"

using namespace NotificationManager;

class NotificationManager::SoundSchemePrivate
{
public:
    QString schemeName;
};

SoundScheme::SoundScheme(QObject *parent)
    : QObject(parent)
    , d(new SoundSchemePrivate())
{
    // TODO check whether we built with it and if it actually works before registering?
    ServerPrivate::get(&Server::self())->registerSoundScheme(this);
}

SoundScheme::~SoundScheme() = default;

QString SoundScheme::schemeName() const
{
    return d->schemeName;
}

void SoundScheme::setSchemeName(const QString &schemeName)
{
    if (d->schemeName != schemeName) {
        d->schemeName = schemeName;
        Q_EMIT schemeNameChanged(schemeName);
    }
}

void SoundScheme::resetSchemeName()
{
    // TODO reset to kdeglobals scheme or freedesktop?
}

EventSound *SoundScheme::soundFromIndex(const QModelIndex &idx)
{
    // TODO check idx model?

    /*if (d->schemeName.isEmpty()) {
        qCWarning(NOTIFICATIONMANAGER) << "Cannot create sound with empty schemeName";
        return nullptr;
    }*/

    const QString eventId = idx.data(Notifications::SoundNameRole).toString();
    const QString path = idx.data(Notifications::SoundFileRole).toString();

    if (eventId.isEmpty() && path.isEmpty()) {
        return nullptr;
    }

    // TODO not too happy with this API and the code duplication here and in the constructor
    auto *sound = new EventSound(d->schemeName, idx);

    return sound;
}
