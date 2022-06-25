/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "dynamicwallpaperupdatetimer.h"

#include <chrono>

#include <QDBusConnection>

#include "clockskewnotifier/clockskewnotifierengine_p.h"
#include "debug.h"

using namespace std::chrono_literals;

DynamicWallpaperUpdateTimer::DynamicWallpaperUpdateTimer(KPackage::ImagePackage *package, QObject *parent)
    : QTimer(parent)
    , m_imagePackage(package)
{
    setInterval(1min);

    connect(this, &DynamicWallpaperUpdateTimer::clockSkewed, this, &DynamicWallpaperUpdateTimer::alignInterval);
}

void DynamicWallpaperUpdateTimer::setActive(bool active)
{
    if (active == isActive()) {
        return;
    }

    if (active) {
        alignInterval();
        start();

        m_engine = ClockSkewNotifierEngine::create(this);

        if (m_engine) {
            connect(m_engine, &ClockSkewNotifierEngine::clockSkewed, this, &DynamicWallpaperUpdateTimer::clockSkewed);
        }

        QDBusConnection::systemBus().connect(QStringLiteral("org.freedesktop.login1"),
                                             QStringLiteral("/org/freedesktop/login1"),
                                             QStringLiteral("org.freedesktop.login1.Manager"),
                                             QStringLiteral("PrepareForSleep"),
                                             this,
                                             SLOT(slotPrepareForSleep(bool)));
    } else {
        stop();

        if (m_engine) {
            m_engine->deleteLater();
            m_engine = nullptr;
        }

        QDBusConnection::systemBus().disconnect(QStringLiteral("org.freedesktop.login1"),
                                                QStringLiteral("/org/freedesktop/login1"),
                                                QStringLiteral("org.freedesktop.login1.Manager"),
                                                QStringLiteral("PrepareForSleep"),
                                                this,
                                                SLOT(slotPrepareForSleep(bool)));
    }
}

void DynamicWallpaperUpdateTimer::alignInterval()
{
    if (!m_imagePackage || !m_imagePackage->isValid()) {
        return;
    }

    setInterval(m_imagePackage->indexAndIntervalAtDateTime(QDateTime::currentDateTime()).second);
    qCDebug(IMAGEWALLPAPER) << "Time to next wallpaper (h):" << interval() / 1000.0 / 60.0 / 60.0;
}

void DynamicWallpaperUpdateTimer::slotPrepareForSleep(bool sleep)
{
    if (sleep || !m_imagePackage) {
        return;
    }

    Q_EMIT clockSkewed();
}
