/*
    SPDX-FileCopyrightText: 2006 Lukas Tinkl <ltinkl@suse.cz>
    SPDX-FileCopyrightText: 2008 Lubos Lunak <l.lunak@suse.cz>
    SPDX-FileCopyrightText: 2009 Ivo Anjo <knuckles@gmail.com>
    SPDX-FileCopyrightText: 2020 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QPointer>
#include <QTimer>

#include <KLocalizedString>
#include <KService>

class KNotification;

class FreeSpaceNotifier : public QObject
{
    Q_OBJECT

public:
    explicit FreeSpaceNotifier(const QString &path, const KLocalizedString &notificationText, QObject *parent = nullptr);
    ~FreeSpaceNotifier() override;

Q_SIGNALS:
    void configureRequested();

private:
    void checkFreeDiskSpace();
    void resetLastAvailable();

    KService::Ptr filelightService() const;
    void exploreDrive();
    void onNotificationClosed();

    QString m_path;
    KLocalizedString m_notificationText;

    QTimer m_timer;
    QTimer *m_lastAvailTimer = nullptr;
    QPointer<KNotification> m_notification;
    qint64 m_lastAvail = -1; // used to suppress repeated warnings when available space hasn't changed
};
