/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sessionmanagement.h"

#include "sessionmanagementbackend.h"

#include <KAuthorized>
#include <KConfigGroup>
#include <KSharedConfig>

#include "kscreenlocker_interface.h"
#include "ksmserver_interface.h"
#include "logoutprompt_interface.h"
#include "screenlocker_interface.h"
#include "shutdown_interface.h"

// add a constructor with the service names and paths pre-populated
class LogoutPromptIface : public OrgKdeLogoutPromptInterface
{
    Q_OBJECT
public:
    LogoutPromptIface()
        : OrgKdeLogoutPromptInterface(QStringLiteral("org.kde.LogoutPrompt"), QStringLiteral("/LogoutPrompt"), QDBusConnection::sessionBus())
    {
    }
};

class ShutdownIface : public OrgKdeShutdownInterface
{
    Q_OBJECT
public:
    ShutdownIface()
        : OrgKdeShutdownInterface(QStringLiteral("org.kde.Shutdown"), QStringLiteral("/Shutdown"), QDBusConnection::sessionBus())
    {
    }
};

SessionManagement::SessionManagement(QObject *parent)
    : QObject(parent)
{
    auto backend = SessionBackend::self();
    connect(backend, &SessionBackend::stateChanged, this, &SessionManagement::stateChanged);
    connect(backend, &SessionBackend::canShutdownChanged, this, &SessionManagement::canShutdownChanged);
    connect(backend, &SessionBackend::canRebootChanged, this, &SessionManagement::canRebootChanged);
    connect(backend, &SessionBackend::canSuspendChanged, this, &SessionManagement::canSuspendChanged);
    connect(backend, &SessionBackend::canHybridSuspendChanged, this, &SessionManagement::canHybridSuspendChanged);
    connect(backend, &SessionBackend::canHibernateChanged, this, &SessionManagement::canHibernateChanged);
    connect(backend, &SessionBackend::aboutToSuspend, this, &SessionManagement::aboutToSuspend);
    connect(backend, &SessionBackend::resumingFromSuspend, this, &SessionManagement::resumingFromSuspend);
}

bool SessionManagement::canShutdown() const
{
    return canLogout() && SessionBackend::self()->canShutdown();
}

bool SessionManagement::canReboot() const
{
    return canLogout() && SessionBackend::self()->canReboot();
}

bool SessionManagement::canLogout() const
{
    // checking both is for compatibility with old kiosk configs
    // authorizeAction is the "correct" one
    return KAuthorized::authorizeAction(QStringLiteral("logout")) && KAuthorized::authorize(QStringLiteral("logout"));
}

bool SessionManagement::canSuspend() const
{
    return SessionBackend::self()->canSuspend();
}

bool SessionManagement::canHybridSuspend() const
{
    return SessionBackend::self()->canHybridSuspend();
}

bool SessionManagement::canHibernate() const
{
    return SessionBackend::self()->canHibernate();
}

bool SessionManagement::canSwitchUser() const
{
    return KAuthorized::authorizeAction(QStringLiteral("start_new_session"));
}

bool SessionManagement::canLock() const
{
    return KAuthorized::authorizeAction(QStringLiteral("lock_screen"));
}

bool SessionManagement::canSaveSession() const
{
    const KConfigGroup c(KSharedConfig::openConfig(QStringLiteral("ksmserverrc")), "General");
    return canLogout() && c.readEntry("loginMode") == QLatin1String("restoreSavedSession");
}

SessionManagement::State SessionManagement::state() const
{
    return SessionBackend::self()->state();
}

void SessionManagement::requestShutdown(ConfirmationMode confirmationMode)
{
    if (!canShutdown()) {
        return;
    }

    if (qEnvironmentVariableIntValue("PLASMA_SESSION_GUI_TEST")) {
        qWarning() << "shutdown";
        return;
    }

    bool confirm = confirmationMode == ConfirmationMode::ForcePrompt;
    if (confirmationMode == ConfirmationMode::Default) {
        confirm = SessionBackend::self()->confirmLogout();
    }
    if (confirm) {
        LogoutPromptIface iface;
        iface.promptShutDown().waitForFinished();
    } else {
        ShutdownIface iface;
        iface.logoutAndShutdown().waitForFinished();
    }
}

void SessionManagement::requestReboot(ConfirmationMode confirmationMode)
{
    if (!canReboot()) {
        return;
    }

    if (qEnvironmentVariableIntValue("PLASMA_SESSION_GUI_TEST")) {
        qWarning() << "reboot";
        return;
    }

    bool confirm = confirmationMode == ConfirmationMode::ForcePrompt;
    if (confirmationMode == ConfirmationMode::Default) {
        confirm = SessionBackend::self()->confirmLogout();
    }
    if (confirm) {
        LogoutPromptIface iface;
        iface.promptReboot().waitForFinished();
    } else {
        ShutdownIface iface;
        iface.logoutAndReboot().waitForFinished();
    }
}

void SessionManagement::requestLogout(ConfirmationMode confirmationMode)
{
    if (!canLogout()) {
        return;
    }
    bool confirm = confirmationMode == ConfirmationMode::ForcePrompt;
    if (confirmationMode == ConfirmationMode::Default) {
        confirm = SessionBackend::self()->confirmLogout();
    }
    if (confirm) {
        LogoutPromptIface iface;
        iface.promptLogout().waitForFinished();
    } else {
        ShutdownIface iface;
        iface.logout().waitForFinished();
    }
}

void SessionManagement::suspend()
{
    if (!canSuspend()) {
        return;
    }
    SessionBackend::self()->suspend();
}

void SessionManagement::hybridSuspend()
{
    if (!canHybridSuspend()) {
        return;
    }
    SessionManagement::hybridSuspend();
}

void SessionManagement::hibernate()
{
    if (!canHibernate()) {
        return;
    }
    SessionBackend::self()->hibernate();
}

void SessionManagement::lock()
{
    if (!canLock()) {
        return;
    }
    OrgFreedesktopScreenSaverInterface iface(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus());
    iface.Lock();
}

void SessionManagement::switchUser()
{
    if (!canSwitchUser()) {
        return;
    }
    OrgKdeScreensaverInterface iface(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/ScreenSaver"), QDBusConnection::sessionBus());
    iface.SwitchUser();
}

void SessionManagement::saveSession()
{
    if (!canSaveSession()) {
        return;
    }
    OrgKdeKSMServerInterfaceInterface ksmserver(QStringLiteral("org.kde.ksmserver"), QStringLiteral("/KSMServer"), QDBusConnection::sessionBus());
    ksmserver.saveCurrentSession();
}

#include "sessionmanagement.moc"
