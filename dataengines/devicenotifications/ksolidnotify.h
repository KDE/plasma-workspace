/*
   Copyright (C) 2010 by Jacopo De Simoi <wilderkde@gmail.com>
   Copyright (C) 2014 by Lukáš Tinkl <ltinkl@redhat.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#ifndef KSOLIDNOTIFY_H
#define KSOLIDNOTIFY_H

#include <QObject>
#include <QHash>
#include <QString>

#include <Solid/Device>
#include <solid/solidnamespace.h>

/**
 * @brief Class which triggers solid notifications
 *
 * This is an internal class which listens to solid errors and route them via dbus to an
 * appropriate visualization (e.g. the plasma device notifier applet); if such visualization is not available
 * errors are shown via regular notifications
 *
 * @author Jacopo De Simoi <wilderkde at gmail.com>
*/

class KSolidNotify : public QObject
{
    Q_OBJECT

public:
    explicit KSolidNotify(QObject *parent);

signals:
    void notify(Solid::ErrorType solidError, const QString& error, const QString& errorDetails, const QString &udi);
    void blockingAppsReady(const QStringList &apps);
    void clearNotification(const QString &udi);

protected slots:
    void onDeviceAdded(const QString &udi);
    void onDeviceRemoved(const QString &udi);

private:
    enum class SolidReplyType {
        Setup,
        Teardown,
        Eject
    };

    void onSolidReply(SolidReplyType type, Solid::ErrorType error, const QVariant &errorData, const QString &udi);

    void connectSignals(Solid::Device* device);
    bool isSafelyRemovable(const QString &udi) const;
    void queryBlockingApps(const QString &devicePath);

    QHash<QString, Solid::Device> m_devices;
};
#endif
