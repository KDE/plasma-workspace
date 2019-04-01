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

#include <QDateTime>
#include <QScopedPointer>
#include <QImage>
#include <QList>
#include <QString>
#include <QUrl>

#include "notifications.h"

#include "notificationmanager_export.h"

namespace NotificationManager
{

/**
 * @short Represents a single notification
 *
 * TODO
 *
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT Notification
{
public:
    explicit Notification(uint id = 0);
    Notification(const Notification &other);
    Notification(Notification &&other) Q_DECL_NOEXCEPT;
    Notification &operator=(const Notification &other);

    ~Notification();

    /*enum class Urgency {
        Low = 0,
        Normal = 1,
        Critical = 2
    };*/

    uint id() const;

    QDateTime created() const;

    QDateTime updated() const;
    void resetUpdated();

    QString summary() const;
    // FIXME remove all those setters as Notification is pretty much immutable
    void setSummary(const QString &summary);

    QString body() const;
    void setBody(const QString &body);

    QString iconName() const;
    void setIconName(const QString &iconName);

    QImage image() const;
    void setImage(const QImage &image);

    QString desktopEntry() const;

    QString notifyRcName() const;
    QString eventId() const;

    QString applicationName() const;
    void setApplicationName(const QString &applicationName);

    QString applicationIconName() const;
    void setApplicationIconName(const QString &applicationIconName);

    QString deviceName() const;

    // should we group the two into a QPair or something?
    QStringList actionNames() const;
    QStringList actionLabels() const;
    bool hasDefaultAction() const;
    void setActions(const QStringList &actions);

    // QVector?
    QList<QUrl> urls() const;
    void setUrls(const QList<QUrl> &urls);

    // FIXME use separate enum again
    Notifications::Urgencies urgency() const;
    void setUrgency(Notifications::Urgencies urgency);

    int timeout() const;
    void setTimeout(int timeout);

    bool configurable() const;
    QString configureActionLabel() const;

    bool expired() const;
    void setExpired(bool expired);

    bool dismissed() const;
    void setDismissed(bool dismissed);

    void processHints(const QVariantMap &hints);

private:
    friend class NotificationModel;
    friend class NotificationServerPrivate;

    class Private;
    Private *d;

};

} // namespace NotificationManager
