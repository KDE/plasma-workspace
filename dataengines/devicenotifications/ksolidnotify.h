/*
    SPDX-FileCopyrightText: 2010 Jacopo De Simoi <wilderkde@gmail.com>
    SPDX-FileCopyrightText: 2014 Lukáš Tinkl <ltinkl@redhat.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QHash>
#include <QObject>
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

Q_SIGNALS:
    void notify(Solid::ErrorType solidError, const QString &error, const QString &errorDetails, const QString &udi);
    void blockingAppsReady(const QStringList &apps);
    void clearNotification(const QString &udi);

protected Q_SLOTS:
    void onDeviceAdded(const QString &udi);
    void onDeviceRemoved(const QString &udi);

private:
    enum class SolidReplyType {
        Setup,
        Teardown,
        Eject,
    };

    void onSolidReply(SolidReplyType type, Solid::ErrorType error, const QVariant &errorData, const QString &udi);

    void connectSignals(Solid::Device *device);
    bool isSafelyRemovable(const QString &udi) const;
    void queryBlockingApps(const QString &devicePath);

    QHash<QString, Solid::Device> m_devices;
};