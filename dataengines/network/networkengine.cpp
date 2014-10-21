/*
 *   Copyright (C) 2007 Percy Leonhardt <percy@eris23.de>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "networkengine.h"

#include <stdio.h>
#include <string.h>

#include <QMap>
#include <QDir>
#include <QTimer>
#include <QRegExp>
#include <QDateTime>
#include <QStringList>
#include <QNetworkAddressEntry>

#include <QDebug>
#include <KLocale>
#include <kio/global.h>
#include <Plasma/DataContainer>
#include <solid/control/networkmanager.h>
#include <solid/control/wirelessnetworkinterface.h>
#include <solid/control/wirelessaccesspoint.h>

//#define SYSPATH "/sys/class/net/"

NetworkEngine::NetworkEngine(QObject* parent, const QVariantList& args)
    : Plasma::DataEngine(parent, args),
      m_secondsSinceLastUpdate(0)
{
    Q_UNUSED(args)
    setMinimumPollingInterval(1000);
}

NetworkEngine::~NetworkEngine()
{
}

QStringList NetworkEngine::sources() const
{
    Solid::Control::NetworkInterfaceList iflist = Solid::Control::NetworkManager::networkInterfaces();

    QStringList availableInterfaces;
    foreach (Solid::Control::NetworkInterface *iface, iflist) {
        availableInterfaces.append(iface->uni().section('/', -1));
    }
    return availableInterfaces;
}

bool NetworkEngine::sourceRequestEvent(const QString &name)
{
    Solid::Control::NetworkInterfaceList iflist = Solid::Control::NetworkManager::networkInterfaces();

    foreach(Solid::Control::NetworkInterface *iface, iflist) {
        if (name == iface->uni().section('/', -1)) {
            setData(name, DataEngine::Data());
            setData(name, I18N_NOOP("UNI"), iface->uni());
            updateSourceEvent(name);
            return true;
        }
    }
    return false;
}

bool NetworkEngine::updateSourceEvent(const QString &source)
{
    const QString uni = query(source)[I18N_NOOP("UNI")].toString();
    Solid::Control::NetworkInterface *iface = Solid::Control::NetworkManager::findNetworkInterface(uni);
    if (!iface) {
        // remove the source as the interface is no longer available
        removeSource(source);
        return false;
    }

    // Check if it is a wireless interface.
    bool isWireless = iface->type() == Solid::Control::NetworkInterface::Ieee80211;
    setData(source, I18N_NOOP("Wireless"), isWireless);
    Solid::Control::WirelessNetworkInterface *wlan = 0;

    if (isWireless) {
        wlan = static_cast<Solid::Control::WirelessNetworkInterface*>(iface);

        QStringList availableNetworks;
        Solid::Control::AccessPointList apList = wlan->accessPoints();
        foreach (const QString &apId, apList) {
            Solid::Control::AccessPoint *ap = wlan->findAccessPoint(apId);
            availableNetworks += ap->ssid();
        }
        setData(source, I18N_NOOP("Available networks"), availableNetworks);
    }

    Solid::Control::NetworkInterface::ConnectionState connectionState = iface->connectionState();
    if (connectionState == Solid::Control::NetworkInterface::Activated) {
        // update the interface
        updateInterfaceData(source, iface);
        if (isWireless) {
            updateWirelessData(source, wlan);
        }
    } else if (connectionState != Solid::Control::NetworkInterface::Activated &&
               query(source)[I18N_NOOP("ConnectionState")].toString() == I18N_NOOP("Activated")) {
        // the interface was disconnected
        removeAllData(source);
        setData(source, I18N_NOOP("UNI"), uni);
    }

    switch (connectionState) {
    case Solid::Control::NetworkInterface::Preparing:
        setData(source, I18N_NOOP("ConnectionState"), "Preparing");
        break;
    case Solid::Control::NetworkInterface::Configuring:
        setData(source, I18N_NOOP("ConnectionState"), "Configuring");
        break;
    case Solid::Control::NetworkInterface::NeedAuth:
        setData(source, I18N_NOOP("ConnectionState"), "NeedAuth");
        break;
    case Solid::Control::NetworkInterface::IPConfig:
        setData(source, I18N_NOOP("ConnectionState"), "IPConfig");
        break;
    case Solid::Control::NetworkInterface::Activated:
        setData(source, I18N_NOOP("ConnectionState"), "Activated");
        break;
    case Solid::Control::NetworkInterface::Failed:
        setData(source, I18N_NOOP("ConnectionState"), "Failed");
        break;
    case Solid::Control::NetworkInterface::Unmanaged:
        setData(source, I18N_NOOP("ConnectionState"), "Unmanaged");
        break;
    case Solid::Control::NetworkInterface::Unavailable:
        setData(source, I18N_NOOP("ConnectionState"), "Unavailable");
        break;
    default:
        setData(source, I18N_NOOP("ConnectionState"), "UnknownState");
        break;
    }
    return true;
}

void NetworkEngine::updateInterfaceData(const QString &source, const Solid::Control::NetworkInterface *iface)
{
    Q_ASSERT(iface);
    Solid::Control::IPv4Config network = iface->ipV4Config();

#if 0
    if (network.isValid()) {
        setData(source, I18N_NOOP("Broadcast"), network.broadcast());
    } else {
        removeData(source, I18N_NOOP("Broadcast"));
    }
#endif

    QList<Solid::Control::IPv4Address> addresses = network.addresses();
    if (addresses.isEmpty()) {
        removeData(source, I18N_NOOP("Gateway"));
        removeData(source, I18N_NOOP("IP"));
        removeData(source, I18N_NOOP("Subnet mask"));
    } else {
        // FIXME: assumes there is only one network for ethernet
        Solid::Control::IPv4Address address = addresses[0];
        setData(source, I18N_NOOP("Gateway"), address.gateway());
        setData(source, I18N_NOOP("IP"), address.address());
        setData(source, I18N_NOOP("Subnet mask"), address.netMask());
    }
}

void NetworkEngine::updateWirelessData(const QString &source, const Solid::Control::WirelessNetworkInterface *iface)
{
    Q_ASSERT(iface);
    const QString currentAP = iface->activeAccessPoint();

    using namespace Solid::Control;
    AccessPoint *ap = iface->findAccessPoint(currentAP);
    WirelessNetworkInterface::OperationMode mode(WirelessNetworkInterface::Unassociated);
    WirelessNetworkInterface::Capabilities capabilities(WirelessNetworkInterface::NoCapability);

    if (ap) {
        setData(source, I18N_NOOP("Link quality"), ap->signalStrength());
        setData(source, I18N_NOOP("Frequency"), ap->frequency());
        setData(source, I18N_NOOP("ESSID"), ap->ssid());
        setData(source, I18N_NOOP("Bitrate"), ap->maxBitRate());
        setData(source, I18N_NOOP("Accesspoint"), ap->hardwareAddress());
        mode = ap->mode();
    } else {
        setData(source, I18N_NOOP("Accesspoint"), i18n("None"));
        removeData(source, I18N_NOOP("Link quality"));
        removeData(source, I18N_NOOP("Frequency"));
        removeData(source, I18N_NOOP("ESSID"));
        removeData(source, I18N_NOOP("Bitrate"));
    }

    switch (mode) {
    case WirelessNetworkInterface::Unassociated:
        setData(source, I18N_NOOP("Mode"), "Unassociated");
        break;
    case WirelessNetworkInterface::Adhoc:
        setData(source, I18N_NOOP("Mode"), "Adhoc");
        break;
    case WirelessNetworkInterface::Managed:
        setData(source, I18N_NOOP("Mode"), "Managed");
        break;
    case WirelessNetworkInterface::Master:
        setData(source, I18N_NOOP("Mode"), "Master");
        break;
    case WirelessNetworkInterface::Repeater:
        setData(source, I18N_NOOP("Mode"), "Repeater");
        break;
    default:
        setData(source, I18N_NOOP("Mode"), i18n("Unknown"));
        break;
    }

    setData(source, I18N_NOOP("Encryption"),
            (capabilities & Solid::Control::AccessPoint::Privacy) != 0);
}


