/********************************************************************
 KSld - the KDE Screenlocker Daemon
 This file is part of the KDE project.

Copyright (C) 2015 Martin Gräßlin <mgraesslin@kde.org>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/
#include "globalaccel.h"

#include <KKeyServer>
#include <netwm.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QX11Info>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

static const QString s_kglobalAccelService = QStringLiteral("org.kde.kglobalaccel");
static const QString s_componentInterface = QStringLiteral("org.kde.kglobalaccel.Component");

/**
 * Whitelist of the components which are allowed to get global shortcuts.
 * The DBus path of the component is the key for the whitelist.
 * The value for each key contains a list of unique shortcut names which are allowed.
 * This allows to not only restrict on component, but also restrict on the shortcuts.
 * E.g. plasmashell might accept media shortcuts, but not shortcuts for switching the activity.
 **/
static const QMap<QString, QList<QString>> s_shortcutWhitelist{
    {QStringLiteral("/component/kmix"), QList<QString>{QStringLiteral("mute"),
                                                       QStringLiteral("decrease_volume"),
                                                       QStringLiteral("increase_volume")}}
};

static uint g_keyModMaskXAccel = 0;
static uint g_keyModMaskXOnOrOff = 0;

static void calculateGrabMasks()
{
    g_keyModMaskXAccel = KKeyServer::accelModMaskX();
    g_keyModMaskXOnOrOff =
            KKeyServer::modXLock() |
            KKeyServer::modXNumLock() |
            KKeyServer::modXScrollLock() |
            KKeyServer::modXModeSwitch();
}

GlobalAccel::GlobalAccel(QObject *parent)
    : QObject(parent)
{
}

void GlobalAccel::prepare()
{
    // recursion check
    if (m_updatingInformation) {
        return;
    }
    // first ensure that we don't have some left over
    release();

    m_keySymbols = xcb_key_symbols_alloc(QX11Info::connection());
    calculateGrabMasks();

    // fetch all compnents from KGlobalAccel
    m_updatingInformation++;
    auto message = QDBusMessage::createMethodCall(s_kglobalAccelService,
                                                  QStringLiteral("/kglobalaccel"),
                                                  QStringLiteral("org.kde.KGlobalAccel"),
                                                  QStringLiteral("allComponents"));
    QDBusPendingReply<QList<QDBusObjectPath>> async = QDBusConnection::sessionBus().asyncCall(message);
    QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
    connect(callWatcher, &QDBusPendingCallWatcher::finished, this, &GlobalAccel::components);
}

void GlobalAccel::components(QDBusPendingCallWatcher *self)
{
    QDBusPendingReply<QList<QDBusObjectPath>> reply = *self;
    self->deleteLater();
    if (!reply.isValid()) {
        m_updatingInformation--;
        return;
    }
    // go through all components, check whether they are in our whitelist
    // if they are whitelisted we check whether they are active
    for (const auto &path : reply.value()) {
        const QString objectPath = path.path();
        bool whitelisted = false;
        for (auto it = s_shortcutWhitelist.begin(); it != s_shortcutWhitelist.end(); ++it) {
            if (objectPath == it.key()) {
                whitelisted = true;
                break;
            }
        }
        if (!whitelisted) {
            continue;
        }
        auto message = QDBusMessage::createMethodCall(s_kglobalAccelService,
                                                      objectPath,
                                                      s_componentInterface,
                                                      QStringLiteral("isActive"));
        QDBusPendingReply<bool> async = QDBusConnection::sessionBus().asyncCall(message);
        QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
        m_updatingInformation++;
        connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
            [this, objectPath] (QDBusPendingCallWatcher *self) {
                QDBusPendingReply<bool> reply = *self;
                self->deleteLater();
                // filter out inactive components
                if (!reply.isValid() || !reply.value()) {
                    m_updatingInformation--;
                    return;
                }

                // active, whitelisted component: get all shortcuts
                auto message = QDBusMessage::createMethodCall(s_kglobalAccelService,
                                                              objectPath,
                                                              s_componentInterface,
                                                              QStringLiteral("allShortcutInfos"));
                QDBusPendingReply<QList<KGlobalShortcutInfo>> async = QDBusConnection::sessionBus().asyncCall(message);
                QDBusPendingCallWatcher *callWatcher = new QDBusPendingCallWatcher(async, this);
                connect(callWatcher, &QDBusPendingCallWatcher::finished, this,
                    [this, objectPath] (QDBusPendingCallWatcher *self) {
                        m_updatingInformation--;
                        QDBusPendingReply<QList<KGlobalShortcutInfo>> reply = *self;
                        self->deleteLater();
                        if (!reply.isValid()) {
                            return;
                        }
                        // restrict to whitelist
                        QList<KGlobalShortcutInfo> infos;
                        auto whitelist = s_shortcutWhitelist.constFind(objectPath);
                        if (whitelist == s_shortcutWhitelist.constEnd()) {
                            // this should not happen, just for safety
                            return;
                        }
                        const auto s = reply.value();
                        for (auto it = s.begin(); it != s.end(); ++it) {
                            if (whitelist.value().contains((*it).uniqueName())) {
                                infos.append(*it);
                            }
                        }
                        m_shortcuts.insert(objectPath, infos);
                    }
                );
            }
        );
    }
    m_updatingInformation--;
}

void GlobalAccel::release()
{
    m_shortcuts.clear();
    if (m_keySymbols) {
        xcb_key_symbols_free(m_keySymbols);
        m_keySymbols = nullptr;
    }
}

bool GlobalAccel::checkKeyPress(xcb_key_press_event_t *event)
{
    if (!m_keySymbols) {
        return false;
    }
    // based and inspired from code in kglobalaccel_x11.cpp
    xcb_keycode_t keyCodeX = event->detail;
    uint16_t keyModX = event->state & (g_keyModMaskXAccel | KKeyServer::MODE_SWITCH);

    xcb_keysym_t keySymX = xcb_key_press_lookup_keysym(m_keySymbols, event, 0);

    // If numlock is active and a keypad key is pressed, XOR the SHIFT state.
    //  e.g., KP_4 => Shift+KP_Left, and Shift+KP_4 => KP_Left.
    if (event->state & KKeyServer::modXNumLock()) {
        xcb_keysym_t sym = xcb_key_symbols_get_keysym(m_keySymbols, keyCodeX, 0);
        // If this is a keypad key,
        if (sym >= XK_KP_Space && sym <= XK_KP_9) {
            switch (sym) {
            // Leave the following keys unaltered
            // FIXME: The proper solution is to see which keysyms don't change when shifted.
            case XK_KP_Multiply:
            case XK_KP_Add:
            case XK_KP_Subtract:
            case XK_KP_Divide:
                break;

            default:
                keyModX ^= KKeyServer::modXShift();
            }
        }
    }

    int keyCodeQt;
    int keyModQt;
    KKeyServer::symXToKeyQt(keySymX, &keyCodeQt);
    KKeyServer::modXToQt(keyModX, &keyModQt);

    if (keyModQt & Qt::SHIFT && !KKeyServer::isShiftAsModifierAllowed(keyCodeQt)) {
        keyModQt &= ~Qt::SHIFT;
    }

    if ((keyModQt == 0 || keyModQt == Qt::SHIFT) && (keyCodeQt >= Qt::Key_Space && keyCodeQt <= Qt::Key_AsciiTilde)) {
        // security check: we don't allow shortcuts without modifier for "normal" keys
        // this is to prevent a malicious application to grab shortcuts for all keys
        // and by that being able to read out the keyboard
        return false;
    }

    const QKeySequence seq(keyCodeQt | keyModQt);
    // let's check whether we have a mapping shortcut
    for (auto it = m_shortcuts.begin(); it != m_shortcuts.end(); ++it) {
        for (const auto &info : it.value()) {
            if (info.keys().contains(seq)) {
                auto signal = QDBusMessage::createMethodCall(s_kglobalAccelService, it.key(), s_componentInterface, QStringLiteral("invokeShortcut"));
                signal.setArguments(QList<QVariant>{QVariant(info.uniqueName())});
                QDBusConnection::sessionBus().asyncCall(signal);
                return true;
            }
        }
    }
    return false;
}
