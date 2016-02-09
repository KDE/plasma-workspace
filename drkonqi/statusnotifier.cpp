/*
    Copyright (C) 2016 Kai Uwe Broulik <kde@privat.broulik.de>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "statusnotifier.h"

#include <QAction>
#include <QDBusConnectionInterface>
#include <QMenu>
#include <QTimer>

#include <KIdleTime>
#include <KLocalizedString>
#include <KNotification>
#include <KService>
#include <KStatusNotifierItem>

#include "drkonqi.h"
#include "crashedapplication.h"

StatusNotifier::StatusNotifier(QObject *parent)
    : QObject(parent)
    , m_autoCloseTimer(new QTimer(this))
    , m_sni(new KStatusNotifierItem(this))
{
    CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    // this is used for both the SNI tooltip as well as the notification
    m_title = i18nc("Placeholder is an application name; it crashed", "%1 Closed Unexpectedly", crashedApp->name());

    // if nobody bothered to look at the crash after 1 minute, just close
    m_autoCloseTimer->setSingleShot(true);
    m_autoCloseTimer->setInterval(60000);
    m_autoCloseTimer->start();
    connect(m_autoCloseTimer, &QTimer::timeout, this, &StatusNotifier::expired);
    connect(this, &StatusNotifier::activated, this, [this] {
        deleteLater();
    });

    KService::Ptr service = KService::serviceByStorageId(crashedApp->fakeExecutableBaseName());
    if (service) {
        m_iconName = service->icon();
    }

    m_sni->setTitle(m_title);
    m_sni->setIconByName(QStringLiteral("tools-report-bug"));
    m_sni->setStatus(KStatusNotifierItem::Active);
    m_sni->setCategory(KStatusNotifierItem::SystemServices);
    m_sni->setToolTip(!m_iconName.isEmpty() ? m_iconName : m_sni->iconName(), m_sni->title(), i18n("Please report this error to help improve this software."));
    connect(m_sni, &KStatusNotifierItem::activateRequested, this, &StatusNotifier::activated);

    // you cannot turn off that "Do you really want to quit?" message, so we'll add our own below
    m_sni->setStandardActionsEnabled(false);

    QMenu *sniMenu = new QMenu();
    QAction *action = new QAction(QIcon::fromTheme(QStringLiteral("tools-report-bug")),
                                  i18n("Report &Bug"), nullptr);
    connect(action, &QAction::triggered, this, &StatusNotifier::activated);
    sniMenu->addAction(action);
    sniMenu->setDefaultAction(action);

    if (canBeRestarted(crashedApp)) {
        action = new QAction(QIcon::fromTheme(QStringLiteral("system-reboot")),
                             i18n("&Restart Application"), nullptr);
        connect(action, &QAction::triggered, this, [crashedApp] {
            crashedApp->restart();
        });
        // once restarted successfully, disable restart option
        connect(crashedApp, &CrashedApplication::restarted, action, [action](bool success) {
            action->setEnabled(!success);
        });
        sniMenu->addAction(action);
    }

    sniMenu->addSeparator();

    action = new QAction(QIcon::fromTheme(QStringLiteral("application-exit")),
                         i18nc("Allows the user to hide this notifier item", "Hide"), nullptr);
    connect(action, &QAction::triggered, this, &StatusNotifier::expired);
    sniMenu->addAction(action);

    m_sni->setContextMenu(sniMenu);

    // make sure the user doesn't miss the SNI by stopping the auto hide timer when the session becomes idle
    int idleId = KIdleTime::instance()->addIdleTimeout(30000);
    connect(KIdleTime::instance(), static_cast<void (KIdleTime::*)(int)>(&KIdleTime::timeoutReached), this, [this, idleId](int id) {
        if (idleId == id) {
            m_autoCloseTimer->stop();
        }
        // this is apparently needed or else resumingFromIdle is never called
        KIdleTime::instance()->catchNextResumeEvent();
    });
    connect(KIdleTime::instance(), &KIdleTime::resumingFromIdle, this, [this] {
        if (!m_autoCloseTimer->isActive()) {
            m_autoCloseTimer->start();
        }
    });
}

StatusNotifier::~StatusNotifier() = default;

void StatusNotifier::notify()
{
    CrashedApplication *crashedApp = DrKonqi::crashedApplication();

    KNotification *notification = KNotification::event(QStringLiteral("applicationcrash"),
                                                       m_title,
                                                       i18n("Please report this error to help improve this software."),
                                                       !m_iconName.isEmpty() ? m_iconName : QStringLiteral("tools-report-bug"),
                                                       nullptr,
                                                       KNotification::DefaultEvent | KNotification::SkipGrouping);

    QStringList actions = {i18nc("Notification action button, keep short", "Report Bug")};

    bool restartEnabled = canBeRestarted(crashedApp);

    if (restartEnabled) {
        actions << i18nc("Notification action button, keep short", "Restart App");
    }

    notification->setActions(actions);

    connect(notification, static_cast<void (KNotification::*)(unsigned int)>(&KNotification::activated),
            this, [this, crashedApp](int actionIndex) {
        // 0 = default action (NOTE this is not implemented by Plasma, clicking notification popup just closes it)
        // 1 = "Report Bug" button
        // 2 = "Restart App" button
        if (actionIndex == 0 || actionIndex == 1) {
            emit activated();
        } else if (actionIndex == 2) {
            crashedApp->restart();
            // keep status notifier there to allow reporting a bug when user chose to restart app
        }
    });

    // when the SNI disappears you won't be able to interact with the notification anymore anyway, so close it
    connect(this, &StatusNotifier::activated, notification, &KNotification::close);
    connect(this, &StatusNotifier::expired, notification, &KNotification::close);
}

bool StatusNotifier::notificationServiceRegistered()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.Notifications"));
}

bool StatusNotifier::canBeRestarted(CrashedApplication *app)
{
    return !app->hasBeenRestarted() && app->fakeExecutableBaseName() != QLatin1String("drkonqi");
}
