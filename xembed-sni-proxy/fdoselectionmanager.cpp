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
#include "fdoselectionmanager.h"

#include "debug.h"

#include <QCoreApplication>
#include <QHash>
#include <QTimer>

#include <QTextDocument>
#include <QX11Info>

#include <KWindowSystem>
#include <KSelectionOwner>

#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_event.h>
#include <xcb/composite.h>
#include <xcb/damage.h>

#include "xcbutils.h"
#include "sniproxy.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

FdoSelectionManager::FdoSelectionManager():
    QObject(),
    m_selectionOwner(new KSelectionOwner(Xcb::atoms->selectionAtom, -1, this))
{
    qCDebug(SNIPROXY) << "starting";

    //load damage extension
    xcb_connection_t *c = QX11Info::connection();
    xcb_prefetch_extension_data(c, &xcb_damage_id);
    const auto *reply = xcb_get_extension_data(c, &xcb_damage_id);
    if (reply->present) {
        m_damageEventBase = reply->first_event;
        xcb_damage_query_version_unchecked(c, XCB_DAMAGE_MAJOR_VERSION, XCB_DAMAGE_MINOR_VERSION);
    } else {
        //no XDamage means
        qCCritical(SNIPROXY) << "could not load damage extension. Quitting";
        qApp->exit(-1);
    }

    qApp->installNativeEventFilter(this);

    m_selectionOwner->claim(false);
    connect(m_selectionOwner, &KSelectionOwner::claimedOwnership, this, &FdoSelectionManager::onClaimedOwnership);
    connect(m_selectionOwner, &KSelectionOwner::failedToClaimOwnership, this, &FdoSelectionManager::onFailedToClaimOwnership);
    connect(m_selectionOwner, &KSelectionOwner::lostOwnership, this, &FdoSelectionManager::onLostOwnership);
}

FdoSelectionManager::~FdoSelectionManager()
{
    qCDebug(SNIPROXY) << "closing";
    m_selectionOwner->release();
}

void FdoSelectionManager::addDamageWatch(xcb_window_t client)
{
    qCDebug(SNIPROXY) << "adding damage watch for " << client;

    xcb_connection_t *c = QX11Info::connection();
    const auto attribsCookie = xcb_get_window_attributes_unchecked(c, client);

    const auto damageId = xcb_generate_id(c);
    m_damageWatches[client] = damageId;
    xcb_damage_create(c, damageId, client, XCB_DAMAGE_REPORT_LEVEL_NON_EMPTY);

    QScopedPointer<xcb_get_window_attributes_reply_t, QScopedPointerPodDeleter> attr(xcb_get_window_attributes_reply(c, attribsCookie, Q_NULLPTR));
    uint32_t events = XCB_EVENT_MASK_STRUCTURE_NOTIFY;
    if (!attr.isNull()) {
        events = events | attr->your_event_mask;
    }
    // the event mask will not be removed again. We cannot track whether another component also needs STRUCTURE_NOTIFY (e.g. KWindowSystem).
    // if we would remove the event mask again, other areas will break.
    xcb_change_window_attributes(c, client, XCB_CW_EVENT_MASK, &events);

}

bool FdoSelectionManager::nativeEventFilter(const QByteArray& eventType, void* message, long int* result)
{
    Q_UNUSED(result);

    if (eventType != "xcb_generic_event_t") {
        return false;
    }

    xcb_generic_event_t* ev = static_cast<xcb_generic_event_t *>(message);

    const auto responseType = XCB_EVENT_RESPONSE_TYPE(ev);
    if (responseType == XCB_CLIENT_MESSAGE) {
        const auto ce = reinterpret_cast<xcb_client_message_event_t *>(ev);
        if (ce->type == Xcb::atoms->opcodeAtom) {
            switch (ce->data.data32[1]) {
                case SYSTEM_TRAY_REQUEST_DOCK:
                    dock(ce->data.data32[2]);
                    return true;
            }
        }
    } else if (responseType == XCB_UNMAP_NOTIFY) {
        const auto unmappedWId = reinterpret_cast<xcb_unmap_notify_event_t *>(ev)->window;
        if (m_proxies[unmappedWId]) {
            undock(unmappedWId);
        }
    } else if (responseType == m_damageEventBase + XCB_DAMAGE_NOTIFY) {
        const auto damagedWId = reinterpret_cast<xcb_damage_notify_event_t *>(ev)->drawable;
        const auto sniProx = m_proxies[damagedWId];

        Q_ASSERT(sniProx);

        if(sniProx) {
            sniProx->update();
            xcb_damage_subtract(QX11Info::connection(), m_damageWatches[damagedWId], XCB_NONE, XCB_NONE);
        }
    }

    return false;
}

void FdoSelectionManager::dock(xcb_window_t winId)
{
    qCDebug(SNIPROXY) << "trying to dock window " << winId;

    if (m_proxies.contains(winId)) {
        return;
    }

    addDamageWatch(winId);
    m_proxies[winId] = new SNIProxy(winId, this);
}

void FdoSelectionManager::undock(xcb_window_t winId)
{
    qCDebug(SNIPROXY) << "trying to undock window " << winId;

    if (!m_proxies.contains(winId)) {
        return;
    }
    m_proxies[winId]->deleteLater();
    m_proxies.remove(winId);
}

void FdoSelectionManager::onClaimedOwnership()
{
    qCDebug(SNIPROXY) << "Manager selection claimed";
}

void FdoSelectionManager::onFailedToClaimOwnership()
{
    qCWarning(SNIPROXY) << "failed to claim ownership of Systray Manager";
    qApp->exit(-1);
}

void FdoSelectionManager::onLostOwnership()
{
    qCWarning(SNIPROXY) << "lost ownership of Systray Manager";
    qApp->exit(-1);
}


