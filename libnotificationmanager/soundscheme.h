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

class EventSound;
class SoundSchemePrivate;

/**
 * @short TODO
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@broulik.de>
 **/
 // TODO do we want this exported?
class NOTIFICATIONMANAGER_EXPORT SoundScheme : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString schemeName READ schemeName WRITE setSchemeName RESET resetSchemeName NOTIFY schemeNameChanged)

    // TODO a property to check whether canberra is available and working?
    //Q_PROPERTY(bool valid READ isValid CONSTANT)

public:
    explicit SoundScheme(QObject *parent = nullptr);
    ~SoundScheme() override;

    QString schemeName() const;
    void setSchemeName(const QString &schemeName);
    void resetSchemeName();
    Q_SIGNAL void schemeNameChanged(const QString &schemeName);

    // TODO make generic generator function?
    Q_INVOKABLE NotificationManager::EventSound *soundFromIndex(const QModelIndex &idx);

private:
    QScopedPointer<SoundSchemePrivate> d;
};

} // namespace NotificationManager
