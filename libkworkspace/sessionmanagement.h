/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "kworkspace_export.h"
#include <QObject>

/**
 * Public facing API for SessionManagement
 */
class KWORKSPACE_EXPORT SessionManagement : public QObject
{
    Q_OBJECT
    Q_PROPERTY(State state READ state NOTIFY stateChanged)

    Q_PROPERTY(bool canShutdown READ canShutdown NOTIFY canShutdownChanged)
    Q_PROPERTY(bool canReboot READ canReboot NOTIFY canRebootChanged)
    Q_PROPERTY(bool canLogout READ canLogout NOTIFY canLogoutChanged)
    Q_PROPERTY(bool canSuspend READ canSuspend NOTIFY canSuspendChanged)
    Q_PROPERTY(bool canHibernate READ canHibernate NOTIFY canHibernateChanged)
    Q_PROPERTY(bool canSwitchUser READ canSwitchUser NOTIFY canSwitchUserChanged)
    Q_PROPERTY(bool canLock READ canLock NOTIFY canLockChanged)
    Q_PROPERTY(bool canSaveSession READ canSaveSession NOTIFY canSaveSessionChanged)

public:
    enum class State {
        /**
         * The backend is loading canXyz functions may not represent the true state
         */
        Loading,
        /**
         * All loaded
         */
        Ready,
        /**
         * Error creating a suitable backend, no actions will be available
         */
        Error,
    };
    Q_ENUM(State)

    enum class ConfirmationMode {
        /**
         * Obey the user's confirmation setting.
         */
        Default = -1,
        /**
         * Don't confirm, shutdown without asking.
         */
        Skip = 0,
        /**
         * Always confirm, ask even if the user turned it off.
         */
        ForcePrompt = 1,
    };
    Q_ENUM(ConfirmationMode)

    SessionManagement(QObject *parent = nullptr);
    ~SessionManagement() override = default;

    State state() const;

    bool canShutdown() const;
    bool canReboot() const;
    bool canLogout() const;
    bool canSuspend() const;
    bool canHybridSuspend() const;
    bool canHibernate() const;
    bool canSwitchUser() const;
    bool canLock() const;
    bool canSaveSession() const;

public Q_SLOTS:
    /**
     * These requestX methods will either launch a prompt to shutdown or
     * The user may cancel it at any point in the process
     */
    void requestShutdown(ConfirmationMode = ConfirmationMode::Default);
    void requestReboot(ConfirmationMode = ConfirmationMode::Default);
    void requestLogout(ConfirmationMode = ConfirmationMode::Default);

    void suspend();
    void hybridSuspend();
    void hibernate();

    void switchUser();
    void lock();

    void saveSession();

Q_SIGNALS:
    void stateChanged();
    void canShutdownChanged();
    void canRebootChanged();
    void canLogoutChanged();
    void canSuspendChanged();
    void canHybridSuspendChanged();
    void canHibernateChanged();
    void canSwitchUserChanged();
    void canLockChanged();
    void canSaveSessionChanged();

    void aboutToSuspend();
    void resumingFromSuspend();

private:
    void *d; // unused, just reserving the space in case we go ABI stable
};
