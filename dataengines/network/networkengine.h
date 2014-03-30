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

#ifndef NETWORKENGINE_H
#define NETWORKENGINE_H

#include <Plasma/DataEngine>

namespace Solid
{
    namespace Control
    {
        class NetworkInterface;
        class WirelessNetworkInterface;
    } // namespace Control
} // namespace Solid

class NetworkEngine : public Plasma::DataEngine
{
    Q_OBJECT

public:
    NetworkEngine(QObject* parent, const QVariantList& args);
    ~NetworkEngine();

    QStringList sources() const;

protected:
    bool sourceRequestEvent(const QString &name);
    bool updateSourceEvent(const QString& source);

private:
    void updateWirelessData(const QString &source, const Solid::Control::WirelessNetworkInterface *iface);
    void updateInterfaceData(const QString &source, const Solid::Control::NetworkInterface *iface);

    int m_secondsSinceLastUpdate;
};

K_EXPORT_PLASMA_DATAENGINE(network, NetworkEngine)

#endif
