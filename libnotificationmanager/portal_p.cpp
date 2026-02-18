/*
 * SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "portal_p.h"

#include "debug.h"

#include "portalnotificationsadaptor.h"

#include "notification_p.h"

#include "server.h"
#include "utils_p.h"

#include <QDBusConnection>

#include <KStartupInfo>
#include <KWaylandExtras>
#include <KWindowSystem>

using namespace Qt::StringLiterals;

namespace NotificationManager
{

PortalPrivate::PortalPrivate(QObject *parent)
    : QObject(parent)
{
}

PortalPrivate::~PortalPrivate() = default;

PortalPrivate &PortalPrivate::self()
{
    static auto *s_self = new PortalPrivate(qApp);
    return *s_self;
}

bool PortalPrivate::init()
{
    if (m_valid) {
        return true;
    }

    new PortalNotificationsAdaptor(this);

    QDBusConnection sessionBus = QDBusConnection::sessionBus();

    if (!m_dbusObjectValid) { // if already registered, don't fail here
        m_dbusObjectValid = sessionBus.registerObject(u"/org/freedesktop/portal/desktop"_s, this);
    }

    if (!m_dbusObjectValid) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to register Portal Notification DBus object";
        return false;
    }

    // TODO do we need dbus master stuff for portal, too? Our name ("plasmanotify") is supposed to be unique, though.
    // and plasmashell isn't DBus activated.

    if (!sessionBus.interface()->registerService(u"org.freedesktop.impl.portal.desktop.plasmanotify"_s)) {
        qCWarning(NOTIFICATIONMANAGER) << "Failed to register Portal Notification service on DBus";
        return false;
    }

    m_valid = true;
    return true;
}

void PortalPrivate::removeNotification(uint notificationId)
{
    m_idMap.remove(notificationId);
}

void PortalPrivate::invokeAction(uint notificationId, const QString &actionId, const QVariant &target, const QString &response, QWindow *window)
{
    const auto idPair = m_idMap.value(notificationId);

    const auto [appId, portalNotificationId] = idPair;
    if (appId.isEmpty() || portalNotificationId.isEmpty()) {
        qCWarning(NOTIFICATIONMANAGER) << "Trying to invoke action" << actionId << "on notification that doesn't exist";
        return;
    }

    auto activateWithPlatformData = [this, appId, portalNotificationId, actionId, target, response](const QVariantMap &platformData) {
        if (constexpr QLatin1String appPrefix{"app."}; actionId.startsWith(appPrefix)) {
            const QString dbusActionId = actionId.mid(appPrefix.size());
            qCDebug(NOTIFICATIONMANAGER) << "Invoking exported action" << dbusActionId << "on" << appId;

            QString appPath = appId;
            // Do we have a halper for that somewhere?
            appPath.replace('.'_L1, '/'_L1);
            appPath.replace('-'_L1, '_'_L1);
            appPath.prepend('/'_L1);

            QVariantList params;
            if (target.isValid()) {
                params.append(target);
            }
            if (!response.isEmpty()) {
                params.append(response);
            }

            auto message = QDBusMessage::createMethodCall(appId, appPath, u"org.freedesktop.Application"_s, u"ActivateAction"_s);
            message.setArguments({dbusActionId, params, platformData});
            QDBusConnection::sessionBus().call(message, QDBus::NoBlock);
        } else {
            qCDebug(NOTIFICATIONMANAGER) << "Emitting action invoked for" << actionId << "on" << appId;

            QVariantList params;
            if (target.isValid()) {
                params.append(target);
            }
            params.append(platformData);
            if (!response.isEmpty()) {
                params.append(response);
            }

            Q_EMIT ActionInvoked(appId, portalNotificationId, actionId, params);
        }
    };

    if (KWindowSystem::isPlatformWayland()) {
        KWaylandExtras::xdgActivationToken(window, appId).then(this, [activateWithPlatformData](const QString &token) {
            activateWithPlatformData({
                {u"activation-token"_s, token},
            });
        });
    } else {
        KStartupInfoId startupId;
        startupId.initId();
        activateWithPlatformData({
            {u"desktop-startup-id"_s, QString::fromUtf8(startupId.id())},
        });
    }
}

void PortalPrivate::AddNotification(const QString &appId, const QString &portalNotificationId, const QVariantMap &props)
{
    // Does the portal enforce this maybe?
    if (portalNotificationId.isEmpty()) {
        return;
    }

    const auto idPair = QPair(appId, portalNotificationId);
    auto notificationId = m_idMap.key(idPair);
    const bool wasReplaced = notificationId != 0;
    if (wasReplaced) {
        // TODO? umm
    } else {
        // Avoid wrapping around to 0 in case of overflow
        if (!m_highestNotificationId) {
            ++m_highestNotificationId;
        }
        notificationId = m_highestNotificationId;
        ++m_highestNotificationId;
    }

    Notification notification(Notification::Source::Portal, notificationId);

    notification.setSummary(props.value(QStringLiteral("title")).toString());

    if (const QString markupBody = props.value(QStringLiteral("markup-body")).toString(); !markupBody.isEmpty()) {
        notification.setBody(markupBody);
    } else {
        // TODO is this correct? does the portal do any stripping?
        QString body = props.value(QStringLiteral("body")).toString().toHtmlEscaped();
        body.replace('\n'_L1, "<br>"_L1);
        notification.setBody(body);
    }

    // FIXME markup body
    notification.setBody(props.value(QStringLiteral("body")).toString());
    notification.setDesktopEntry(appId);
    notification.setWasAddedDuringInhibition(Server::self().inhibited());

    // TODO dbus service?

    notification.d->processPortalProperties(props);

    const bool showAsNew = props.value(u"display-hints"_s).toStringList().contains(u"show-as-new"_s);

    if (wasReplaced) {
        notification.resetUpdated();
        if (showAsNew) {
            // TODO or should we do a replaced "show as new" so it just doesn't carry over read etc?
            Q_EMIT notificationRemoved(notificationId);
            Q_EMIT notificationAdded(notification);
        } else {
            Q_EMIT notificationReplaced(notificationId, notification);
        }
    } else {
        m_idMap.insert(notificationId, idPair);
        Q_EMIT notificationAdded(notification);
    }

    // TODO NotificationWatcher
}

void PortalPrivate::RemoveNotification(const QString &appId, const QString &portalNotificationId)
{
    const auto idPair = QPair(appId, portalNotificationId);
    const auto notificationId = m_idMap.key(idPair);

    if (notificationId) {
        m_idMap.remove(notificationId);
        Q_EMIT notificationRemoved(notificationId);
    }
}

QVariantMap PortalPrivate::supportedOptions() const
{
    return {
        {u"category"_s,
         QStringList{
             u"im.received"_s,
         }},
        {u"button-purpose"_s,
         QStringList{
             u"im.reply-with-text"_s,
         }},
    };
}

uint PortalPrivate::version() const
{
    return 2;
}

} // namespace NotificationManager

#include "moc_portal_p.cpp"
