/*
 * SPDX-FileCopyrightText: 2024 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "geotimezonemodule.h"

#include <config-workspace.h>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QDBusPendingCallWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRandomGenerator>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTime>
#include <QTimeZone>

#include <NetworkManagerQt/ActiveConnection>
#include <NetworkManagerQt/Manager>

#include <chrono>

#include "geotimezoned_debug.h"
#include "timedated_interface.h"

K_PLUGIN_CLASS_WITH_JSON(KdedGeoTimeZonePlugin, "geotimezoned.json")

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

static const QUrl s_geoIpEndpoint{u"https://geoip.kde.org/v1/timezone"_s};

static constexpr QLatin1String s_timedateService{"org.freedesktop.timedate1"};
static constexpr QLatin1String s_timedatePath{"/org/freedesktop/timedate1"};

KdedGeoTimeZonePlugin::KdedGeoTimeZonePlugin(QObject *parent, const QVariantList &args)
    : KDEDModule(parent)
{
    Q_UNUSED(args);

    m_nam.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    m_nam.setStrictTransportSecurityEnabled(true);
    m_nam.enableStrictTransportSecurityStore(true, QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + "/kded/hsts/"_L1);

    m_delayCheckTimer.setSingleShot(true);
    m_delayCheckTimer.callOnTimeout(this, &KdedGeoTimeZonePlugin::checkTimeZone);

    connect(NetworkManager::notifier(), &NetworkManager::Notifier::connectivityChanged, this, &KdedGeoTimeZonePlugin::scheduleCheckTimeZone);
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::meteredChanged, this, &KdedGeoTimeZonePlugin::scheduleCheckTimeZone);
    connect(NetworkManager::notifier(), &NetworkManager::Notifier::primaryConnectionChanged, this, &KdedGeoTimeZonePlugin::onPrimaryConnectionChanged);

    onPrimaryConnectionChanged();
}

void KdedGeoTimeZonePlugin::onPrimaryConnectionChanged()
{
    const auto connection = NetworkManager::primaryConnection();
    if (!connection) {
        return;
    }

    if (m_state.lastConnectionUuid() == connection->uuid()) {
        return;
    }

    scheduleCheckTimeZone();
}

bool KdedGeoTimeZonePlugin::shouldCheckTimeZone() const
{
    const auto connection = NetworkManager::primaryConnection();
    if (!connection) {
        return false;
    }

    // Deliberately 25 hours instead 24, so it's not the same time of the day every day.
    // When on the same connection, don't check if it hadn't checked before.
    // Always update promptly in case of a different connection, however.
    if (connection->uuid() == m_state.lastConnectionUuid() && (!m_graceTimer.isValid() || m_graceTimer.durationElapsed() < 25h)) {
        return false;
    }

    const auto connectivity = NetworkManager::connectivity();
    if (connectivity == NetworkManager::Connectivity::NoConnectivity || connectivity == NetworkManager::Connectivity::Portal
        || connectivity == NetworkManager::Connectivity::Limited) {
        return false;
    }

    const auto metered = NetworkManager::metered();
    if (metered == NetworkManager::Device::MeteredStatus::Yes || metered == NetworkManager::Device::MeteredStatus::GuessYes) {
        return false;
    }

    // VPN endpoint could be in a completely different location.
    if (connection->vpn()) {
        return false;
    }

    return true;
}

void KdedGeoTimeZonePlugin::scheduleCheckTimeZone()
{
    if (!shouldCheckTimeZone()) {
        m_delayCheckTimer.stop();
        return;
    }

    if (m_delayCheckTimer.isActive()) {
        return;
    }

    // Start checking after a random delay to spread out requests.
    const std::chrono::seconds checkDelay{QRandomGenerator::global()->bounded(60, 300)};
    qCDebug(GEOTIMEZONED_DEBUG) << "Scheduled update in" << checkDelay.count() << "s";
    m_delayCheckTimer.start(checkDelay);
}

void KdedGeoTimeZonePlugin::checkTimeZone()
{
    if (!shouldCheckTimeZone()) {
        return;
    }

    refresh();
}

void KdedGeoTimeZonePlugin::setGeoTimeZone(const QByteArray &geoTimeZoneId)
{
    org::freedesktop::timedate1 timedateInterface{s_timedateService, s_timedatePath, QDBusConnection::systemBus()};

    const QByteArray currentTimeZoneId = timedateInterface.timezone().toLatin1();
    if (currentTimeZoneId.isEmpty()) {
        qCWarning(GEOTIMEZONED_DEBUG) << "Failed to get current system time zone from timedated";
        return;
    }

    // Not caching the current time zone as a member since it could have changed elsewhere (e.g. in the KCM).
    if (currentTimeZoneId == geoTimeZoneId) {
        qCDebug(GEOTIMEZONED_DEBUG) << "Time zone" << geoTimeZoneId << "is the same as the current time zone";
        return;
    }

    const QTime oldTime = QTime::currentTime();

    qCInfo(GEOTIMEZONED_DEBUG) << "Automatically changing time zone to" << geoTimeZoneId << "based on current location";
    // Not really ideal to allow interactive authorization as it will potentially cause an unsolicited
    // PolKit prompt, depending on distro configuration.
    // On the other hand the feature will not work at all otherwise.
    auto reply = timedateInterface.SetTimezone(QString::fromLatin1(geoTimeZoneId), true /*interactive*/);
    auto *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, geoTimeZoneId, oldTime](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();

        QDBusPendingReply<> reply = *watcher;
        if (reply.isError()) {
            qCWarning(GEOTIMEZONED_DEBUG) << "Failed to set time zone to" << geoTimeZoneId << reply.error().message();
            return;
        }

        if (const auto connection = NetworkManager::primaryConnection()) {
            m_state.setLastConnectionUuid(connection->uuid());
            m_state.save();
        }

        QTimeZone timeZone(geoTimeZoneId);

        QString timeZoneName = QString::fromLatin1(geoTimeZoneId);
        if (timeZone.isValid()) { // Can this ever be false since we did succeed in setting it?
            timeZoneName = timeZone.displayName(QDateTime::currentDateTime(), QTimeZone::LongName);
        }

        const QTime newTime = QTime::currentTime();
        const QString displayTime = QLocale().toString(newTime, QLocale().timeFormat(QLocale::ShortFormat));

        // Show OSD that clock or time zone got changed, depending on whether the time differs.
        QString timeZoneOsdText;
        if (newTime.hour() == oldTime.hour() && newTime.minute() == oldTime.minute()) {
            timeZoneOsdText = i18nc("OSD, keep short", "Time zone changed to %1", timeZoneName);
        } else {
            timeZoneOsdText = i18nc("System clock was changed due to time zone change OSD, keep short: new time (time zone)",
                                    "Clock changed to %1 (%2)",
                                    displayTime,
                                    timeZoneName);
        }

        QDBusMessage msg = QDBusMessage::createMethodCall(u"org.kde.plasmashell"_s, u"/org/kde/osdService"_s, u"org.kde.osdService"_s, u"showText"_s);
        msg.setArguments({u"clock"_s, timeZoneOsdText});
        QDBusConnection::sessionBus().call(msg, QDBus::NoBlock);
    });
}

void KdedGeoTimeZonePlugin::refresh()
{
    if (calledFromDBus()) {
        if (m_pendingRefresh) {
            qCInfo(GEOTIMEZONED_DEBUG) << "Refresh already in progress";
            sendErrorReply(QDBusError::LimitsExceeded, i18n("Refresh is already in progress."));
            return;
        }

        qCInfo(GEOTIMEZONED_DEBUG) << "Refresh requested via DBus";
    }

    // Only update last connection UUID when actually checking.
    // This saves a check when connecting to a different network only briefly and switching back.
    const auto connection = NetworkManager::primaryConnection();
    if (!connection) {
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::NoNetwork);
        }
        return;
    }

    if (calledFromDBus()) {
        setDelayedReply(true);
        m_pendingRefresh = message();
    }

    QNetworkRequest request(s_geoIpEndpoint);
    request.setPriority(QNetworkRequest::LowPriority);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

    // NOTE QApplication::applicationVersion is kded (KDE Frameworks) version but geotimezoned is shipped with Plasma.
    const QString userAgent = u"KDE/Plasma/geotimezoned/"_s + QLatin1String(WORKSPACE_VERSION_STRING);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

    auto *reply = m_nam.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();

        auto cleanupPendingRefresh = qScopeGuard([this] {
            m_pendingRefresh.reset();
        });

        if (reply->error() != QNetworkReply::NoError) {
            qCWarning(GEOTIMEZONED_DEBUG) << "Failed to load time zone from" << reply->url() << reply->errorString();
            if (m_pendingRefresh) {
                auto dbusError = m_pendingRefresh->createErrorReply(QDBusError::Failed, reply->errorString());
                QDBusConnection::sessionBus().send(dbusError);
            }
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument replyDoc = QJsonDocument::fromJson(reply->readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(GEOTIMEZONED_DEBUG) << "JSON parse error" << parseError.errorString();
            if (m_pendingRefresh) {
                auto dbusError = m_pendingRefresh->createErrorReply(QDBusError::Failed, parseError.errorString());
                QDBusConnection::sessionBus().send(dbusError);
            }
            return;
        }

        const QJsonObject replyObject = replyDoc.object();
        const QString timeZone = replyObject.value("time_zone"_L1).toString();
        if (timeZone.isEmpty()) {
            qCWarning(GEOTIMEZONED_DEBUG) << "Received no or an invalid time zone object" << replyObject;
            if (m_pendingRefresh) {
                auto dbusError = m_pendingRefresh->createErrorReply(QDBusError::Failed, i18n("Received no or an invalid time zone."));
                QDBusConnection::sessionBus().send(dbusError);
            }
            return;
        }

        qCInfo(GEOTIMEZONED_DEBUG) << "Received time zone" << timeZone;
        setGeoTimeZone(timeZone.toLatin1());
        m_graceTimer.restart();

        if (m_pendingRefresh) {
            auto dbusReply = m_pendingRefresh->createReply(timeZone);
            QDBusConnection::sessionBus().send(dbusReply);
        }
    });
}

#include "geotimezonemodule.moc"
