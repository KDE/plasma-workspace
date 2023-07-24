/*
 * SPDX-FileCopyrightText: 2024 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KDEDModule>

#include <QByteArray>
#include <QDBusContext>
#include <QDBusMessage>
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QTimer>

#include <optional>

#include "geotimezonedstate.h"

class KdedGeoTimeZonePlugin : public KDEDModule, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.geotimezoned")

public:
    KdedGeoTimeZonePlugin(QObject *parent, const QVariantList &args);

    Q_SCRIPTABLE void refresh();

private:
    void onPrimaryConnectionChanged();
    bool shouldCheckTimeZone() const;
    void scheduleCheckTimeZone();
    void checkTimeZone();
    void setGeoTimeZone(const QByteArray &geoTimeZoneId);

    QNetworkAccessManager m_nam;
    QElapsedTimer m_graceTimer;
    GeotimezonedState m_state;
    QTimer m_delayCheckTimer;
    std::optional<QDBusMessage> m_pendingRefresh;
};
