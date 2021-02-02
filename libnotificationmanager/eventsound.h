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

#pragma once

#include "notificationmanager_export.h"

#include <QObject>

class QString;

namespace NotificationManager
{

class EventSoundPrivate;

/**
 * @short TODO
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@broulik.de>
 **/
// TODO do we want this exported?
class NOTIFICATIONMANAGER_EXPORT EventSound : public QObject
{
    Q_OBJECT

    // TODO properties

public:
    explicit EventSound(QObject *parent = nullptr);
    // TODO not too happy with this API
    EventSound(const QString &schemeName, const QModelIndex &idx, QObject *parent = nullptr);
    ~EventSound() override;

    Q_INVOKABLE void play();

private:
    QScopedPointer<EventSoundPrivate> d;
};

} // namespace NotificationManager
