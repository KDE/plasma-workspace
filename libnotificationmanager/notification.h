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
 * @author Kai Uwe Broulik <kde@privat.broulik.de>
 **/
class NOTIFICATIONMANAGER_EXPORT Notification
{
public:
    explicit Notification(uint id = 0);

    Notification(const Notification &other);
    Notification(Notification &&other) Q_DECL_NOEXCEPT;

    Notification &operator=(const Notification &other);
    Notification &operator=(Notification &&other) Q_DECL_NOEXCEPT;

    virtual ~Notification();

    uint id() const;

    QString dBusService() const;
    void setDBusService(const QString &dBusService);

    QDateTime created() const;

    QDateTime updated() const;
    void resetUpdated();

    bool read() const;
    void setRead(bool read);

    QString summary() const;
    void setSummary(const QString &summary);

    QString body() const;
    void setBody(const QString &body);

    // This returns the raw body data as provided by the notification
    // this is useful when you want to html sanitization at different
    // stage then the notification server.
    QString rawBody() const;

    QString icon() const;
    void setIcon(const QString &icon);

    QImage image() const;
    void setImage(const QImage &image);

    QString desktopEntry() const;
    void setDesktopEntry(const QString &desktopEntry);

    QString notifyRcName() const;
    QString eventId() const;

    QString applicationName() const;
    void setApplicationName(const QString &applicationName);

    QString applicationIconName() const;
    void setApplicationIconName(const QString &applicationIconName);

    QString originName() const;

    // should we group the two into a QPair or something?
    QStringList actionNames() const;
    QStringList actionLabels() const;
    bool hasDefaultAction() const;
    QString defaultActionLabel() const;
    void setActions(const QStringList &actions);

    QList<QUrl> urls() const;
    void setUrls(const QList<QUrl> &urls);

    // FIXME use separate enum again
    Notifications::Urgency urgency() const;
    void setUrgency(Notifications::Urgency urgency);

    bool userActionFeedback() const;

    int timeout() const;
    void setTimeout(int timeout);

    bool configurable() const;
    QString configureActionLabel() const;

    bool hasReplyAction() const;
    QString replyActionLabel() const;
    QString replyPlaceholderText() const;
    QString replySubmitButtonText() const;
    QString replySubmitButtonIconName() const;

    QString category() const;

    bool expired() const;
    void setExpired(bool expired);

    bool dismissed() const;
    void setDismissed(bool dismissed);

    // Little bit of mess here, we want to sometime keep track of processed hints, and not process it.
    QVariantMap hints() const;
    void setHints(const QVariantMap &hints);

    void processHints(const QVariantMap &hints);

private:
    friend class NotificationsModel;
    friend class AbstractNotificationsModel;
    friend class ServerPrivate;

    class Private;
    Private *d;
};

} // namespace NotificationManager
