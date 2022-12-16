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

#pragma once

// TODO: need this?
#include "coreprotocol.h"

namespace MockCompositor
{
class DataOffer;

class DataDeviceManager : public Global, public QtWaylandServer::wl_data_device_manager
{
    Q_OBJECT
public:
    explicit DataDeviceManager(CoreCompositor *compositor, int version = 1)
        : QtWaylandServer::wl_data_device_manager(compositor->m_display, version)
        , m_version(version)
    {
    }
    ~DataDeviceManager() override
    {
        qDeleteAll(m_dataDevices);
    }
    bool isClean() override;
    DataDevice *deviceFor(Seat *seat);

    int m_version = 1; // TODO: remove on libwayland upgrade
    QMap<Seat *, DataDevice *> m_dataDevices;

protected:
    void data_device_manager_get_data_device(Resource *resource, uint32_t id, ::wl_resource *seatResource) override;
};

class DataDevice : public QtWaylandServer::wl_data_device
{
public:
    explicit DataDevice(DataDeviceManager *manager, Seat *seat)
        : m_manager(manager)
        , m_seat(seat)
    {
    }
    ~DataDevice() override;
    void send_data_offer(::wl_resource *resource) = delete;
    DataOffer *sendDataOffer(::wl_client *client, const QStringList &mimeTypes = {});

    void send_selection(::wl_resource *resource) = delete;
    void sendSelection(DataOffer *offer);

    DataDeviceManager *m_manager = nullptr;
    Seat *m_seat = nullptr;
    QVector<DataOffer *> m_sentSelectionOffers;

protected:
    void data_device_release(Resource *resource) override
    {
        int removed = m_manager->m_dataDevices.remove(m_seat);
        QVERIFY(removed);
        wl_resource_destroy(resource->handle);
    }
};

class DataOffer : public QObject, public QtWaylandServer::wl_data_offer
{
    Q_OBJECT
public:
    explicit DataOffer(DataDevice *dataDevice, ::wl_client *client, int version)
        : QtWaylandServer::wl_data_offer(client, 0, version)
        , m_dataDevice(dataDevice)
    {
    }

    DataDevice *m_dataDevice = nullptr;

signals:
    void receive(QString mimeType, int fd);

protected:
    void data_offer_destroy_resource(Resource *resource) override;
    void data_offer_receive(Resource *resource, const QString &mime_type, int32_t fd) override;
    //    void data_offer_accept(Resource *resource, uint32_t serial, const QString &mime_type) override;
    void data_offer_destroy(Resource *resource) override;
};

} // namespace MockCompositor
