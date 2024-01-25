/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sessionmanagement.h"

#include "sessionmanagementbackend.h"

#include <KAuthorized>
#include <KConfigGroup>
#include <KSharedConfig>

#include <iostream>

#include "ksmserver_interface.h"
#include "logoutprompt_interface.h"
#include "screenlocker_interface.h"
#include "shutdown_interface.h"

#include "libkworkspace_debug.h"

using namespace Qt::StringLiterals;

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
    connect(backend, &SessionBackend::canSuspendThenHibernateChanged, this, &SessionManagement::canSuspendThenHibernateChanged);
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

bool SessionManagement::canSuspendThenHibernate() const
{
    return SessionBackend::self()->canSuspendThenHibernate();
}

bool SessionManagement::canSwitchUser() const
{
    return KAuthorized::authorizeAction(QStringLiteral("start_new_session")) && SessionBackend::self()->canSwitchUser();
}

bool SessionManagement::canLock() const
{
    return KAuthorized::authorizeAction(QStringLiteral("lock_screen"));
}

bool SessionManagement::canSaveSession() const
{
    const KConfigGroup c(KSharedConfig::openConfig(u"ksmserverrc"_s), u"General"_s);
    return canLogout() && c.readEntry("loginMode") == QLatin1String("restoreSavedSession");
}

SessionManagement::State SessionManagement::state() const
{
    return SessionBackend::self()->state();
}

void SessionManagement::requestLogoutPrompt()
{
    if (qEnvironmentVariableIntValue("PLASMA_SESSION_GUI_TEST")) {
        std::cout << "show logout prompt " << std::endl;
        return;
    }

    // Don't bother to check for whether the user normally wants confirmation or
    // not; if this function was invoked, it means they do want to see the logout
    // prompt right now
    LogoutPromptIface iface;
    iface.promptAll().waitForFinished();
}

void SessionManagement::requestShutdown(ConfirmationMode confirmationMode)
{
    if (!canShutdown()) {
        return;
    }

    if (qEnvironmentVariableIntValue("PLASMA_SESSION_GUI_TEST")) {
        std::cout << "shutdown" << std::endl;
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
        std::cout << "reboot" << std::endl;
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
    SessionBackend::self()->hybridSuspend();
}

void SessionManagement::hibernate()
{
    if (!canHibernate()) {
        return;
    }
    SessionBackend::self()->hibernate();
}

void SessionManagement::suspendThenHibernate()
{
    if (!canSuspendThenHibernate()) {
        return;
    }
    SessionBackend::self()->suspendThenHibernate();
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
    if (!canSwitchUser() || !canLock()) {
        return;
    }

    if (!qEnvironmentVariableIsSet("XDG_SEAT_PATH")) {
        qCWarning(LIBKWORKSPACE_DEBUG) << "Cannot switch user: XDG_SEAT_PATH not set";
        return;
    }

    // lock first
    OrgFreedesktopScreenSaverInterface screenSaverIface(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                        QStringLiteral("/ScreenSaver"),
                                                        QDBusConnection::sessionBus());
    QDBusPendingReply<> pendingLock = screenSaverIface.Lock();

    // then tell the display manager to switch
    auto watcher = new QDBusPendingCallWatcher(pendingLock, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, watcher, &QObject::deleteLater);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [](QDBusPendingCallWatcher *watcher) {
        if (watcher->isError()) {
            qCWarning(LIBKWORKSPACE_DEBUG) << "Failed to lock screen before switching user:" << watcher->error().message();
            return;
        }
        QDBusMessage switchToGreeterMessage = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.DisplayManager"),
                                                                             qgetenv("XDG_SEAT_PATH"),
                                                                             QStringLiteral("org.freedesktop.DisplayManager.Seat"),
                                                                             "SwitchToGreeter");

        QDBusConnection::systemBus().asyncCall(switchToGreeterMessage);
    });
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
