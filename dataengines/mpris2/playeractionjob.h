/*
    SPDX-FileCopyrightText: 2008 Alex Merry <alex.merry@kdemail.net>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

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
