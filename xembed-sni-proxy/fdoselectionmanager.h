/*
    Registers as a embed container
    SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#pragma once

#include <QAbstractNativeEventFilter>
#include <QHash>
#include <QObject>

#include <xcb/xcb.h>

class KSelectionOwner;
class SNIProxy;

class FdoSelectionManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    FdoSelectionManager();
    ~FdoSelectionManager() override;

Q_SIGNALS:
    void ready();

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
