/*
 * Copyright 2008  Alex Merry <alex.merry@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef PLAYERACTIONJOB_H
#define PLAYERACTIONJOB_H

#include <Plasma/ServiceJob>

#include "playercontrol.h"

class QDBusPendingCallWatcher;
class QDBusPendingCall;
class QDBusVariant;

class PlayerActionJob : public Plasma::ServiceJob
{
    Q_OBJECT

public:
    PlayerActionJob(const QString &operation, QMap<QString, QVariant> &parameters, PlayerControl *parent);

    enum {
        /**
         * The media player reports that the operation is not possible
         */
        Denied = UserDefinedError,
        /**
         * Calling the media player resulted in an error
         */
        Failed,
        /**
         * An argument is missing or of wrong type
         *
         * errorText is argument name
         */
        MissingArgument,
        /**
         * The operation name is unknown
         */
        UnknownOperation,
    };

    void start() override;

    QString errorString() const override;

private Q_SLOTS:
    void callFinished(QDBusPendingCallWatcher *);
    void setDBusProperty(const QString &iface, const QString &propName, const QDBusVariant &value);

private:
    void listenToCall(const QDBusPendingCall &call);

    QPointer<PlayerControl> m_controller;
};

#endif // PLAYERACTIONJOB_H
