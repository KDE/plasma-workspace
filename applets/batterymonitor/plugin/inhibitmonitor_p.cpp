/*
 * SPDX-FileCopyrightText: 2024 Bohdan Onofriichuk <bogdan.onofriuchuk@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "inhibitmonitor_p.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QGuiApplication>
#include <QString>

InhibitMonitor &InhibitMonitor::self()
{
    static InhibitMonitor inhibitMonitor;
    return inhibitMonitor;
}

InhibitMonitor::InhibitMonitor()
{
}

InhibitMonitor::~InhibitMonitor()
{
    if (m_sleepInhibitionCookie) {
        stopSuppressingSleep(true);
    }
    if (m_lockInhibitionCookie) {
        stopSuppressingScreenPowerManagement();
    }
}

bool InhibitMonitor::getInhibit()
{
    return m_lockInhibitionCookie || m_sleepInhibitionCookie;
}

void InhibitMonitor::inhibit(const QString &reason, bool isSilent)
{
    beginSuppressingSleep(reason, isSilent);
    beginSuppressingScreenPowerManagement(reason);
}

void InhibitMonitor::uninhibit(bool isSilent)
{
    stopSuppressingSleep(isSilent);
    stopSuppressingScreenPowerManagement();
}

void InhibitMonitor::beginSuppressingSleep(const QString &reason, bool isSilent)
{
    qDebug() << "Begin Suppresing sleep signal arrived";
    if (m_sleepInhibitionCookie) { // an inhibition request is already active; don't trigger another one
        Q_EMIT isManuallyInhibitedChanged(true);
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                      QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("Inhibit"));
    msg << QGuiApplication::desktopFileName() << reason;
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, isSilent](QDBusPendingCallWatcher *watcher) {
        QDBusReply<uint> reply = *watcher;
        if (reply.isValid()) {
            m_sleepInhibitionCookie = reply.value();
            if (!isSilent) {
                qDebug() << "Begin Suppresing sleep signal is used";
                QDBusMessage osdMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                                     QStringLiteral("/org/kde/osdService"),
                                                                     QStringLiteral("org.kde.osdService"),
                                                                     QStringLiteral("powerManagementInhibitedChanged"));
                osdMsg << true;
                QDBusConnection::sessionBus().asyncCall(osdMsg);
            }
            Q_EMIT isManuallyInhibitedChanged(true);
        } else {
            Q_EMIT isManuallyInhibitedChangeError(false);
        }
        watcher->deleteLater();
    });
}

void InhibitMonitor::stopSuppressingSleep(bool isSilent)
{
    qDebug() << "Stop Suppresing sleep signal arrived";
    if (!m_sleepInhibitionCookie) {
        Q_EMIT isManuallyInhibitedChanged(false);
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                                      QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                                      QStringLiteral("UnInhibit"));
    msg << m_sleepInhibitionCookie.value();
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, isSilent](QDBusPendingCallWatcher *watcher) {
        QDBusReply<void> reply = *watcher;
        if (reply.isValid()) {
            m_sleepInhibitionCookie.reset();
            if (!isSilent) {
                qDebug() << "Stop Suppresing sleep signal is used";
                QDBusMessage osdMsg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.plasmashell"),
                                                                     QStringLiteral("/org/kde/osdService"),
                                                                     QStringLiteral("org.kde.osdService"),
                                                                     QStringLiteral("powerManagementInhibitedChanged"));
                osdMsg << false;
                QDBusConnection::sessionBus().asyncCall(osdMsg);
            }
            Q_EMIT isManuallyInhibitedChanged(false);
        } else {
            Q_EMIT isManuallyInhibitedChangeError(true);
        }
        watcher->deleteLater();
    });
}

void InhibitMonitor::beginSuppressingScreenPowerManagement(const QString &reason)
{
    if (m_lockInhibitionCookie) { // an inhibition request is already active; don't trigger another one
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("/ScreenSaver"),
                                                      QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("Inhibit"));
    msg << QGuiApplication::desktopFileName() << reason;
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusReply<uint> reply = *watcher;
        if (reply.isValid()) {
            m_lockInhibitionCookie = reply.value();
        }
        watcher->deleteLater();
    });
}

void InhibitMonitor::stopSuppressingScreenPowerManagement()
{
    if (!m_lockInhibitionCookie) {
        return;
    }

    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("/ScreenSaver"),
                                                      QStringLiteral("org.freedesktop.ScreenSaver"),
                                                      QStringLiteral("UnInhibit"));
    msg << m_lockInhibitionCookie.value();
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msg);
    auto *watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        QDBusReply<void> reply = *watcher;
        if (reply.isValid()) {
            m_lockInhibitionCookie.reset();
        }
        watcher->deleteLater();
    });
}
