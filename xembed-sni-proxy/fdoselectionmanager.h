/*
 * Registers as a embed container
 * Copyright (C) 2015 <davidedmundson@kde.org> David Edmundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef FDOSELECTIONMANAGER_H
#define FDOSELECTIONMANAGER_H

#include <QObject>
#include <QHash>
#include <QAbstractNativeEventFilter>

#include <xcb/xcb.h>

class KSelectionOwner;
class SNIProxy;

class FdoSelectionManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    FdoSelectionManager();
    ~FdoSelectionManager() override;

protected:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

private Q_SLOTS:
    void onClaimedOwnership();
    void onFailedToClaimOwnership();
    void onLostOwnership();

private:
    void init();
    bool addDamageWatch(xcb_window_t client);
    void dock(xcb_window_t embed_win);
    void undock(xcb_window_t client);
    void setSystemTrayVisual();

    uint8_t m_damageEventBase;

    QHash<xcb_window_t, u_int32_t> m_damageWatches;
    QHash<xcb_window_t, SNIProxy *> m_proxies;
    KSelectionOwner *m_selectionOwner;
};

#endif
