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
#ifndef GLOBALACCEL_H
#define GLOBALACCEL_H

#include <KGlobalShortcutInfo>

#include <QObject>
#include <QMap>

class QDBusPendingCallWatcher;

struct xcb_key_press_event_t;
typedef struct _XCBKeySymbols xcb_key_symbols_t;

/**
 * @short Interaction with KGlobalAccel.
 *
 * While the screen is locked, we want a few white listed global shortcuts to activate.
 * While the screen is locked KGlobalAcceld is not functional as the screen locker holds an
 * active X11 key grab and KGlobalAcceld does not get the key events. This prevents useful keys,
 * like volume control to no longer function.
 *
 * This class circumvents the problem by interacting with KGlobalAccel when the screen is locked
 * to still allow a few white listed shortcuts (like volume control) to function.
 *
 * As the screen is locked we can operate on a few assumptions which simplifies the interaction:
 * shortcuts won't change. The user cannot interact with the system thus the global shortcut won't
 * change while the screen is locked. This allows us to fetch the allowed shortcut information from
 * KGlobalAcceld when the screen gets locked and keep these information around till the screen is
 * unlocked. We do not need to update the information while the screen is locked.
 *
 * As the information is fetched in an async way from KGlobalAcceld there is a short time window
 * when the screen is locked, but the shortcut information is not fetched. This is considered a
 * not relevant corner case as we can assume that right when the screen locks (due to e.g. idle)
 * no user is in front of the system and is not able to press the shortcut or that the user directly
 * wants to cancel the lock screen again (grace time).
 *
 * Components are just registered by name in KGlobalAccel. This would in theory allow a malicious
 * application to register under a white listed name with white listed shortcuts and bind enough
 * shortcuts to turn this functionality into a key grabber to read the users password. To prevent such
 * attacks the shortcuts which can be invoked are restricted: the triggered key may not be a single
 * key in alphanumeric area with or without the shift keys. If a global shortcut contains an
 * alphanumeric key it will only be handled if also an additional modifier like Alt of Meta is pressed.
 **/
class GlobalAccel : public QObject
{
    Q_OBJECT
public:
    GlobalAccel(QObject *parent = nullptr);

    /**
     * Starts interacting with KGlobalAccel and fetches the up-to-date shortcut information.
     **/
    void prepare();
    /**
     * Discards all knowing shortcut information.
     **/
    void release();

    /**
     * Checks whether a global shortcut is triggered for the given @p event.
     * If there is a global shortcut it gets invoked and @c true is returned.
     * If there is no matching global shortcut @c false is returned.
     **/
    bool checkKeyPress(xcb_key_press_event_t *event);

private:
    void components(QDBusPendingCallWatcher *watcher);
    /**
     * Recursion check: for each DBus call to KGlobalAccel this counter is
     * increased, on each reply decreased. As long as we have running DBus
     * calls, we do not enter prepare again.
     *
     * This ensures that if the screen gets locked, unlocked, locked in a short
     * time while we are still fetching information from KGlobalAccel we do not
     * enter an incorrect state.
     **/
    uint m_updatingInformation = 0;
    /**
     * The shortcuts which got fetched from KGlobalAccel and can be operated on.
     * The key of contains the component DBus object path, the value the list of
     * allowed shortcuts.
     **/
    QMap<QString, QList<KGlobalShortcutInfo>> m_shortcuts;
    xcb_key_symbols_t *m_keySymbols = nullptr;
};

#endif
