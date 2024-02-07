/*
 * SPDX-FileCopyrightText: 2019 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nightcolorinhibitor.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(NIGHTCOLOR_CONTROL, "org.kde.plasma.nightcolorcontrol")

static const QString s_serviceName = QStringLiteral("org.kde.KWin.NightLight");
static const QString s_path = QStringLiteral("/org/kde/KWin/NightLight");
static const QString s_interface = QStringLiteral("org.kde.KWin.NightLight");

class NightColorInhibitor::Private
{
public:
    uint cookie = 0;
    State state = Uninhibited;
    bool pendingUninhibit = false;
};

NightColorInhibitor::NightColorInhibitor(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

NightColorInhibitor::~NightColorInhibitor()
{
    uninhibit();
}

NightColorInhibitor::State NightColorInhibitor::state() const
{
    return d->state;
}

void NightColorInhibitor::inhibit()
{
    if (d->state == Inhibited) {
        return;
    }

    d->pendingUninhibit = false;

    if (d->state == Inhibiting) {
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, s_path, s_interface, QStringLiteral("inhibit"));

    QDBusPendingReply<uint> cookie = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(cookie, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        const bool wasPendingUninhibit = d->pendingUninhibit;
        d->pendingUninhibit = false;

        const QDBusPendingReply<uint> reply = *self;
        self->deleteLater();

        if (reply.isError()) {
            qCWarning(NIGHTCOLOR_CONTROL()) << "Could not inhibit Night Color:" << reply.error().message();
            d->state = Uninhibited;
            Q_EMIT stateChanged();
            return;
        }

        d->cookie = reply.value();
        d->state = Inhibited;
        Q_EMIT stateChanged();

        if (wasPendingUninhibit) {
            uninhibit();
        }
    });

    d->state = Inhibiting;
    Q_EMIT stateChanged();
}

void NightColorInhibitor::uninhibit()
{
    if (d->state == Uninhibiting || d->state == Uninhibited) {
        return;
    }

    if (d->state == Inhibiting) {
        d->pendingUninhibit = true;
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(s_serviceName, s_path, s_interface, QStringLiteral("uninhibit"));
    message.setArguments({d->cookie});

    QDBusPendingReply<void> reply = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *self) {
        self->deleteLater();

        if (d->state != Uninhibiting) {
            return;
        }

        const QDBusPendingReply<void> reply = *self;
        if (reply.isError()) {
            qCWarning(NIGHTCOLOR_CONTROL) << "Could not uninhibit Night Color:" << reply.error().message();
        }

        d->state = Uninhibited;
        Q_EMIT stateChanged();
    });

    d->state = Uninhibiting;
    Q_EMIT stateChanged();
}
