/*
    SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDBusArgument>
#include <QDateTime>
#include <QImage>
#include <QList>
#include <QString>
#include <QUrl>

#include <KService>

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
    static void sanitizeImage(QImage &image);

    void loadImagePath(const QString &path);

    static QString defaultComponentName();
    static QSize maximumImageSize();

    static KService::Ptr serviceForDesktopEntry(const QString &desktopEntry);

    void setDesktopEntry(const QString &desktopEntry);
    void processHints(const QVariantMap &hints);

    void setUrgency(Notifications::Urgency urgency);

    uint id = 0;
    // Bus name of the creator/sender
    QString dBusService;
    QDateTime created;
    QDateTime updated;
    bool read = false;

    QString summary;
    QString body;
    // raw body text without sanitize called.
    QString rawBody;
    // Can be theme icon name or path
    QString icon;
    QImage image;

    QString applicationName;
    QString desktopEntry;
    bool configurableService = false;
    QString serviceName; // "Name" field in KService from desktopEntry
    QString applicationIconName;

    QString originName;

    QStringList actionNames;
    QStringList actionLabels;
    bool hasDefaultAction = false;
    QString defaultActionLabel;

    bool hasConfigureAction = false;
    QString configureActionLabel;

    bool configurableNotifyRc = false;
    QString notifyRcName;
    QString eventId;

    bool hasReplyAction = false;
    QString replyActionLabel;
    QString replyPlaceholderText;
    QString replySubmitButtonText;
    QString replySubmitButtonIconName;

    QString category;
    QString xdgTokenAppId;

    QList<QUrl> urls;
    QVariantMap hints = QVariantMap();

    bool userActionFeedback = false;
    Notifications::Urgency urgency = Notifications::NormalUrgency;
    int timeout = -1;

    bool expired = false;
    bool dismissed = false;

    bool resident = false;
    bool transient = false;
};

} // namespace NotificationManager
