/* This file is part of the KDE Project
   Copyright (c) 2006 Lukas Tinkl <ltinkl@suse.cz>
   Copyright (c) 2008 Lubos Lunak <l.lunak@suse.cz>
   Copyright (c) 2009 Ivo Anjo <knuckles@gmail.com>
   Copyright (c) 2020 Kai Uwe Broulik <kde@broulik.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _FREESPACENOTIFIER_H_
#define _FREESPACENOTIFIER_H_

#include <QTimer>
#include <QPointer>

#include <KLocalizedString>
#include <KService>

class KNotification;

class FreeSpaceNotifier : public QObject
{
    Q_OBJECT

public:
    explicit FreeSpaceNotifier(const QString &path, const KLocalizedString &notificationText, QObject *parent = nullptr);
    ~FreeSpaceNotifier() override;

signals:
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

#endif
