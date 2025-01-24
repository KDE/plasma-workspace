/*
    SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "sessionmanagement.h"

#include "sessionmanagementbackend.h"

#include <KAuthorized>
#include <KConfigGroup>
#include <KSharedConfig>

#include <QDBusPendingCallWatcher>
#include <QEventLoopLocker>

#include <iostream>

#include "logoutprompt_interface.h"
#include "screenlocker_interface.h"
#include "shutdown_interface.h"

#include "libkworkspace_debug.h"

using namespace Qt::StringLiterals;

static void lockQuitUntilFinished(QDBusPendingCall pendingCall)
{
    auto watcher = new QDBusPendingCallWatcher(pendingCall);
    QEventLoopLocker eventLoopLocker;

    // Keep event loop locker alive whist the call is in progress to keep application running

    // If the recipient is dbus activated and the sender quits before the target is up
    // the method may not be dispatched.
    // See https://gitlab.freedesktop.org/dbus/dbus/-/issues/72
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, watcher, [watcher, eventLoopLocker = std::move(eventLoopLocker)]() {
        watcher->deleteLater();
    });
}

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

    KConfigWatcher::Ptr saveSessionWatcher = KConfigWatcher::create(KSharedConfig::openConfig(u"ksmserverrc"_s));
    connect(saveSessionWatcher.data(), &KConfigWatcher::configChanged, this, [this](const KConfigGroup &group, const QByteArrayList &names) {
        if (group.name() == u"General" && names.contains("loginMode")) {
            Q_EMIT canSaveSessionChanged();
        }
    });
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
        std::cout << "show logout screen " << std::endl;
        return;
    }

    // Don't bother to check for whether the user normally wants confirmation or
    // not; if this function was invoked, it means they do want to see the logout
    // prompt right now
    LogoutPromptIface iface;
    lockQuitUntilFinished(iface.promptAll());
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
        lockQuitUntilFinished(iface.promptShutDown());

    } else {
        ShutdownIface iface;
        lockQuitUntilFinished(iface.logoutAndShutdown());
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
        lockQuitUntilFinished(iface.promptReboot());
    } else {
        ShutdownIface iface;
        lockQuitUntilFinished(iface.logoutAndReboot());
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
        lockQuitUntilFinished(iface.promptLogout());
    } else {
        ShutdownIface iface;
        lockQuitUntilFinished(iface.logout());
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
                                                                             qEnvironmentVariable("XDG_SEAT_PATH"),
                                                                             QStringLiteral("org.freedesktop.DisplayManager.Seat"),
                                                                             u"SwitchToGreeter"_s);

        QDBusConnection::systemBus().asyncCall(switchToGreeterMessage);
    });
}

void SessionManagement::saveSession()
{
    if (!canSaveSession()) {
        return;
    }
    ShutdownIface iface;
    iface.saveSession();
}

#include "sessionmanagement.moc"

#include "moc_sessionmanagement.cpp"
