/*  This file is part of the KDE project
    Copyright (C) 2008 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) version 3.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "deviceinfo.h"
#include "hardwaredatabase.h"

#include <kconfiggroup.h>
#include <kdebug.h>
#include <klocale.h>

namespace PS
{

bool DeviceKey::operator==(const DeviceKey& rhs) const
{
    if (uniqueId.isNull() || rhs.uniqueId.isNull()) {
        return cardNumber == rhs.cardNumber && deviceNumber == rhs.deviceNumber;
    }

    return
        uniqueId == rhs.uniqueId &&
        cardNumber == rhs.cardNumber &&
        deviceNumber == rhs.deviceNumber;
}

DeviceInfo::DeviceInfo()
    : m_index(0), m_initialPreference(0), m_isAvailable(false), m_isAdvanced(true),
    m_dbNameOverrideFound(false)
{
    m_type = Unspecified;
}

DeviceInfo::DeviceInfo(Type t, const QString &cardName, const QString &icon,
        const DeviceKey &key, int pref, bool adv) :
    m_type(t),
    m_cardName(cardName),
    m_icon(icon),
    m_key(key),
    m_index(0),
    m_initialPreference(pref),
    m_isAvailable(false),
    m_isAdvanced(adv),
    m_dbNameOverrideFound(false)
{
    applyHardwareDatabaseOverrides();
}

bool DeviceInfo::operator<(const DeviceInfo& rhs) const
{
    return m_initialPreference > rhs.m_initialPreference;
}

bool DeviceInfo::operator==(const DeviceInfo& rhs) const
{
    return m_key == rhs.m_key;
}

const QString& DeviceInfo::name() const
{
    return m_cardName;
}

void DeviceInfo::setPreferredName(const QString& name)
{
    if (!m_dbNameOverrideFound) {
        m_cardName = name;
    }
}

int DeviceInfo::index() const
{
    return m_index;
}

const QString DeviceInfo::description() const
{
    if (!m_isAvailable) {
        return i18n("<html>This device is currently not available (either it is unplugged or the "
                "driver is not loaded).</html>");
    }

    QString list;
    foreach (const DeviceAccess &a, m_accessList) {
        foreach (const QString &id, a.deviceIds()) {
            list += i18nc("The first argument is name of the driver/sound subsystem. "
                    "The second argument is the device identifier", "<li>%1: %2</li>",
                    a.driverName(), id);
        }
    }

    return i18n("<html>This will try the following devices and use the first that works: "
            "<ol>%1</ol></html>", list);
}

const QString& DeviceInfo::icon() const
{
    return m_icon;
}

bool DeviceInfo::isAvailable() const
{
    return m_isAvailable;
}

bool DeviceInfo::isAdvanced() const
{
    return m_isAdvanced;
}

int DeviceInfo::initialPreference() const
{
    return m_initialPreference;
}

int DeviceInfo::deviceNumber() const
{
    return m_key.deviceNumber;
}

const QList< DeviceAccess >& DeviceInfo::accessList() const
{
    return m_accessList;
}

const DeviceKey& DeviceInfo::key() const
{
    return m_key;
}

void DeviceInfo::addAccess(const DeviceAccess &access)
{
    m_isAvailable |= !access.deviceIds().isEmpty();

    m_accessList << access;
    qSort(m_accessList); // FIXME: do sorted insert
}

void DeviceInfo::applyHardwareDatabaseOverrides()
{
    // now let's take a look at the hardware database whether we have to override something
    kDebug(601) << "looking for" << m_key.uniqueId;
    if (HardwareDatabase::contains(m_key.uniqueId)) {
        const HardwareDatabase::Entry &e = HardwareDatabase::entryFor(m_key.uniqueId);
        kDebug(601) << "  found it:" << e.name << e.iconName << e.initialPreference << e.isAdvanced;

        if (!e.name.isEmpty()) {
            m_dbNameOverrideFound = true;
            m_cardName = e.name;
        }

        if (!e.iconName.isEmpty()) {
            m_icon = e.iconName;
        }

        if (e.isAdvanced != 2) {
            m_isAdvanced = e.isAdvanced;
        }

        m_initialPreference = e.initialPreference;
    }
}

const QString DeviceInfo::prefixForConfigGroup() const
{
    QString groupPrefix;
    if (m_type == Audio) {
        groupPrefix = "AudioDevice_";
    }
    if (m_type == Video) {
        groupPrefix = "VideoDevice_";
    }

    return groupPrefix;
}

void DeviceInfo::removeFromCache(const KSharedConfigPtr &config) const
{
    if (m_type == Unspecified)
        return;

    KConfigGroup cGroup(config, prefixForConfigGroup().toLatin1() + m_key.uniqueId);
    cGroup.writeEntry("deleted", true);
}

void DeviceInfo::syncWithCache(const KSharedConfigPtr &config)
{
    if (m_type == Unspecified) {
        kWarning(601) << "Device info for" << name() << "has unspecified type, unable to sync with cache";
        return;
    }

    KConfigGroup cGroup(config, prefixForConfigGroup().toLatin1() + m_key.uniqueId);
    if (cGroup.exists()) {
        m_index = cGroup.readEntry("index", 0);
    }

    if (m_index >= 0) {
        KConfigGroup globalGroup(config, "Globals");
        m_index = -globalGroup.readEntry("nextIndex", 1);
        globalGroup.writeEntry("nextIndex", 1 - m_index);
        Q_ASSERT(m_index < 0);

        cGroup.writeEntry("index", m_index);
    }

    cGroup.writeEntry("cardName", m_cardName);
    cGroup.writeEntry("iconName", m_icon);
    cGroup.writeEntry("initialPreference", m_initialPreference);
    cGroup.writeEntry("isAdvanced", m_isAdvanced);
    cGroup.writeEntry("deviceNumber", m_key.deviceNumber);
    cGroup.writeEntry("deleted", false);

    bool hotpluggable = false;

    // HACK #1: only internal soundcards should get the icon audio-card. All others, we assume, are
    // hotpluggable
    hotpluggable |= m_icon != QLatin1String("audio-card");

    // HACK #2: Solid currently offers audio-card icon for some hotpluggable stuff, so #1 is unreliable
    hotpluggable |= (bool) m_cardName.contains("usb", Qt::CaseInsensitive);
    hotpluggable |= (bool) m_cardName.contains("headset", Qt::CaseInsensitive);
    hotpluggable |= (bool) m_cardName.contains("headphone", Qt::CaseInsensitive);

    cGroup.writeEntry("hotpluggable", hotpluggable);
}

} // namespace PS
