/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2024 Natalie Clarius <natalie.clarius@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nightlightinhibitor.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QLoggingCategory>
#include <memory>

Q_LOGGING_CATEGORY(NIGHTLIGHT_CONTROL, "org.kde.plasma.nightlightcontrol")

static const QString s_serviceName = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_path = QStringLiteral("/org/kde/KWin/NightLight");
static const QString s_interface = QStringLiteral("org.kde.KWin.NightLight");

NightLightInhibitor::NightLightInhibitor(QObject *parent)
    : QObject(parent)
{
}

NightLightInhibitor::~NightLightInhibitor()
{
    uninhibit();
}

NightLightInhibitor &NightLightInhibitor::instance()
{
    static NightLightInhibitor nightLightInhibitor;

    return nightLightInhibitor;
}

bool NightLightInhibitor::isInhibited() const
{
    return m_state == Inhibited || m_state == Inhibiting || m_pendingUninhibit;
}

void NightLightInhibitor::toggleInhibition()
{
    if (isInhibited()) {
        uninhibit();
    } else {
        inhibit();
    }
}

void NightLightInhibitor::inhibit()
{
    if (m_state == Inhibited) {
        return;
    }

    m_pendingUninhibit = false;

    if (m_state == Inhibiting) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, s_path, s_interface, QStringLiteral("inhibit"));

    QDBusPendingReply<uint> cookie = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(cookie, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        const bool wasPendingUninhibit = m_pendingUninhibit;
        m_pendingUninhibit = false;

        const QDBusPendingReply<uint> reply = *self;
        self->deleteLater();

        if (reply.isError()) {
            qCWarning(NIGHTLIGHT_CONTROL()) << "Could not inhibit Night Light:" << reply.error().message();
            m_state = Uninhibited;
            Q_EMIT inhibitedChanged();
            return;
        }

        m_cookie = reply.value();
        m_state = Inhibited;
        Q_EMIT inhibitedChanged();

        if (wasPendingUninhibit) {
            uninhibit();
        }
    });

    m_state = Inhibiting;
}

void NightLightInhibitor::uninhibit()
{
    if (m_state == Uninhibiting || m_state == Uninhibited) {
        return;
    }

    if (m_state == Inhibiting) {
        m_pendingUninhibit = true;
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, s_path, s_interface, QStringLiteral("uninhibit"));
    message.setArguments({m_cookie});

    QDBusPendingReply<void> reply = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        self->deleteLater();

        if (m_state != Uninhibiting) {
            return;
        }

        const QDBusPendingReply<void> reply = *self;
        if (reply.isError()) {
            qCWarning(NIGHTLIGHT_CONTROL) << "Could not uninhibit Night Light:" << reply.error().message();
        }

        m_state = Uninhibited;
        Q_EMIT inhibitedChanged();
    });

    m_state = Uninhibiting;
}

#include "moc_nightlightinhibitor.cpp"
