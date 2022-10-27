/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "datadevice.h"

namespace MockCompositor
{
bool DataDeviceManager::isClean()
{
    for (auto *device : qAsConst(m_dataDevices)) {
        // The client should not leak selection offers, i.e. if this fails, there is a missing
        // data_offer.destroy request
        if (!device->m_sentSelectionOffers.empty())
            return false;
    }
    return true;
}

DataDevice *DataDeviceManager::deviceFor(Seat *seat)
{
    Q_ASSERT(seat);
    if (auto *device = m_dataDevices.value(seat, nullptr))
        return device;

    auto *device = new DataDevice(this, seat);
    m_dataDevices[seat] = device;
    return device;
}

void DataDeviceManager::data_device_manager_get_data_device(Resource *resource, uint32_t id, wl_resource *seatResource)
{
    auto *seat = fromResource<Seat>(seatResource);
    QVERIFY(seat);
    auto *device = deviceFor(seat);
    device->add(resource->client(), id, resource->version());
}

DataDevice::~DataDevice()
{
    // If the client(s) hasn't deleted the wayland object, just ignore subsequent events
    for (auto *r : resourceMap())
        wl_resource_set_implementation(r->handle, nullptr, nullptr, nullptr);
}

DataOffer *DataDevice::sendDataOffer(wl_client *client, const QStringList &mimeTypes)
{
    Q_ASSERT(client);
    auto *offer = new DataOffer(this, client, m_manager->m_version);
    for (auto *resource : resourceMap().values(client))
        wl_data_device::send_data_offer(resource->handle, offer->resource()->handle);
    for (const auto &mimeType : mimeTypes)
        offer->send_offer(mimeType);
    return offer;
}

void DataDevice::sendSelection(DataOffer *offer)
{
    auto *client = offer->resource()->client();
    for (auto *resource : resourceMap().values(client))
        wl_data_device::send_selection(resource->handle, offer->resource()->handle);
    m_sentSelectionOffers << offer;
}

void DataOffer::data_offer_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    delete this;
}

void DataOffer::data_offer_receive(Resource *resource, const QString &mime_type, int32_t fd)
{
    Q_UNUSED(resource);
    emit receive(mime_type, fd);
}

void DataOffer::data_offer_destroy(QtWaylandServer::wl_data_offer::Resource *resource)
{
    bool removed = m_dataDevice->m_sentSelectionOffers.removeOne(this);
    QVERIFY(removed);
    wl_resource_destroy(resource->handle);
}

} // namespace MockCompositor
