/*
    SPDX-FileCopyrightText: 2021 Aleix Pol Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "virtualkeyboard.h"

#include <QDBusPendingCallWatcher>

KwinVirtualKeyboardInterface::KwinVirtualKeyboardInterface()
    : OrgKdeKwinVirtualKeyboardInterface(QStringLiteral("org.kde.KWin"), QStringLiteral("/VirtualKeyboard"), QDBusConnection::sessionBus())
{
    // Connect to the signals that can change the value of willShowOnActive, since we don't get a
    // signal for it directly.
    //
    // Should be refactored to add a signal for willShowOnActive in KWin.
    connect(this, &OrgKdeKwinVirtualKeyboardInterface::availableChanged, this, &KwinVirtualKeyboardInterface::refreshWillShowOnActive);
    connect(this, &OrgKdeKwinVirtualKeyboardInterface::activeClientSupportsTextInputChanged, this, &KwinVirtualKeyboardInterface::refreshWillShowOnActive);
    connect(this, &OrgKdeKwinVirtualKeyboardInterface::modeChanged, this, &KwinVirtualKeyboardInterface::refreshWillShowOnActive);
    refreshWillShowOnActive();
}

// Wrap the D-Bus call in a Q_INVOKABLE method, otherwise the qml tooling doesn't know about this.
void KwinVirtualKeyboardInterface::forceActivate()
{
    OrgKdeKwinVirtualKeyboardInterface::forceActivate();
}

bool KwinVirtualKeyboardInterface::willShowOnActive() const
{
    return m_willShowOnActive;
}

void KwinVirtualKeyboardInterface::refreshWillShowOnActive()
{
    auto *watcher = new QDBusPendingCallWatcher(OrgKdeKwinVirtualKeyboardInterface::willShowOnActive(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        const QDBusPendingReply<bool> reply = *watcher;
        if (reply.isError()) {
            return;
        }
        if (m_willShowOnActive != reply.value()) {
            m_willShowOnActive = reply.value();
            Q_EMIT willShowOnActiveChanged();
        }
    });
}

#include "moc_virtualkeyboard.cpp"
