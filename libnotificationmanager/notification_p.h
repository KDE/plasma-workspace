/*
 * Copyright 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
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

#include <QDBusArgument>
#include <QDateTime>
#include <QScopedPointer>
#include <QImage>
#include <QList>
#include <QString>
#include <QUrl>

#include "notifications.h"

namespace NotificationManager
{

class Q_DECL_HIDDEN Notification::Private
{
public:
    Private();
    ~Private();

    static QString sanitize(const QString &text);
    static QImage decodeNotificationSpecImageHint(const QDBusArgument &arg);
    static QString findImageForSpecImagePath(const QString &_path);

    static QString defaultComponentName();

    void processHints(const QVariantMap &hints);

    void setUrgency(Notifications::Urgencies urgency);

    uint id = 0;
    QDateTime created;
    QDateTime updated;

    QString summary;
    QString body;
    QString iconName;
    QImage image;

    QString applicationName;
    QString desktopEntry;
    QString serviceName; // TODO really needed?
    bool configurableService = false;
    QString applicationIconName;

    QString deviceName;

    QStringList actionNames;
    QStringList actionLabels;
    bool hasDefaultAction = false;

    bool hasConfigureAction = false;
    QString configureActionLabel;

    bool configurableNotifyRc = false;
    QString notifyRcName;
    QString eventId;

    QList<QUrl> urls;

    // FIXME use separate enum again
    Notifications::Urgencies urgency = Notifications::NormalUrgency;
    int timeout = -1;

    bool expired = false;
    bool dismissed = false;
};

} // namespace NotificationManager
