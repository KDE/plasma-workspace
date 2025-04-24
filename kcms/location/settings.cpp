/*
    SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "settings.h"

#include <KAuth/Action>
#include <KAuth/ExecuteJob>

#include <QDebug>

GeoClueSettings::GeoClueSettings(QObject *parent)
    : QObject(parent)
{
}

bool GeoClueSettings::isDefaults() const
{
    return m_enabled.current == m_enabled.defaults
        && m_staticLocation.current == m_staticLocation.defaults;
}

bool GeoClueSettings::isSaveNeeded() const
{
    return m_enabled.current != m_enabled.loaded
        || m_staticLocation.current != m_staticLocation.loaded;
}

bool GeoClueSettings::isEnabled() const
{
    return m_enabled.current;
}

void GeoClueSettings::setEnabled(bool enabled)
{
    if (m_enabled.current != enabled) {
        m_enabled.current = enabled;
        Q_EMIT enabledChanged();
        Q_EMIT changed();
    }
}

StaticLocation GeoClueSettings::staticLocation() const
{
    return m_staticLocation.current;
}

void GeoClueSettings::setStaticLocation(const StaticLocation &location)
{
    if (m_staticLocation.current != location) {
        m_staticLocation.current = location;
        Q_EMIT staticLocationChanged();
        Q_EMIT changed();
    }
}

static bool isStaticLocationEnabled(const QStringList &sources)
{
    if (sources.size() != 1) {
        return false;
    } else {
        return sources.first() == QLatin1String("static-source");
    }
}

void GeoClueSettings::load()
{
    KAuth::Action query(QStringLiteral("org.kde.kcm.location.query"));
    query.setHelperId(QStringLiteral("org.kde.kcm.location"));
    KAuth::ExecuteJob *job = query.execute();
    if (!job->exec()) {
        qWarning() << "Failed to query geoclue configuration:" << job->errorString();
        return;
    }

    const QVariantMap data = job->data();
    qDebug() << data;
    const QStringList enabledSources = data[QLatin1String("enabledSources")].toStringList();
    const QVariantMap staticLocation = data[QLatin1String("staticLocation")].toMap();

    m_enabled.loaded = !enabledSources.isEmpty();
    if (m_enabled.current != m_enabled.loaded) {
        m_enabled.current = m_enabled.loaded;
        Q_EMIT enabledChanged();
    }

    m_staticLocation.loaded.present = isStaticLocationEnabled(enabledSources);
    m_staticLocation.loaded.coordinate = QGeoCoordinate(staticLocation[QStringLiteral("latitude")].toDouble(),
                                                        staticLocation[QStringLiteral("longitude")].toDouble(),
                                                        staticLocation[QStringLiteral("altitude")].toDouble());
    if (m_staticLocation.current != m_staticLocation.loaded) {
        m_staticLocation.current = m_staticLocation.loaded;
        Q_EMIT staticLocationChanged();
    }
}

void GeoClueSettings::save()
{
    bool reconfigure = false;

    if (m_enabled.loaded != m_enabled.current) {
        m_enabled.loaded = m_enabled.current;
        reconfigure = true;

        KAuth::Action action(m_enabled.current ? QStringLiteral("org.kde.kcm.location.enable") : QStringLiteral("org.kde.kcm.location.disable"));
        action.setHelperId(QStringLiteral("org.kde.kcm.location"));

        KAuth::ExecuteJob *job = action.execute();
        if (!job->exec()) {
            qWarning() << "Failed to toggle geolocation status:" << job->errorString();
        }
    }

    if (m_staticLocation.loaded != m_staticLocation.current) {
        m_staticLocation.loaded = m_staticLocation.current;
        reconfigure = true;

        if (!m_staticLocation.current.present) {
            KAuth::Action action(QStringLiteral("org.kde.kcm.location.unsetstaticlocation"));
            action.setHelperId(QStringLiteral("org.kde.kcm.location"));

            KAuth::ExecuteJob *job = action.execute();
            if (!job->exec()) {
                qWarning() << "Failed to turn on automatic location detection:" << job->errorString();
            }
        } else {
            KAuth::Action action(QStringLiteral("org.kde.kcm.location.setstaticlocation"));
            action.setHelperId(QStringLiteral("org.kde.kcm.location"));
            action.setArguments({
                {QStringLiteral("latitude"), QVariant::fromValue(m_staticLocation.current.coordinate.latitude())},
                {QStringLiteral("longitude"), QVariant::fromValue(m_staticLocation.current.coordinate.longitude())},
                {QStringLiteral("altitude"), QVariant::fromValue(m_staticLocation.current.coordinate.altitude())},
            });

            KAuth::ExecuteJob *job = action.execute();
            if (!job->exec()) {
                qWarning() << "Failed to set static location:" << job->errorString();
            }
        }
    }

    if (reconfigure) {
        KAuth::Action action(QStringLiteral("org.kde.kcm.location.reconfigure"));
        action.setHelperId(QStringLiteral("org.kde.kcm.location"));

        KAuth::ExecuteJob *job = action.execute();
        if (!job->exec()) {
            qWarning() << "Failed to reload geoclue:" << job->errorString();
        }
    }
}

void GeoClueSettings::defaults()
{
    setEnabled(m_enabled.defaults);
    setStaticLocation(m_staticLocation.defaults);
}

#include "moc_settings.cpp"
