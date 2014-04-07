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

#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include "deviceaccess.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>

#include <ksharedconfig.h>

namespace PS
{

/**
 * \brief Identifies a certain device.
 *
 * Unique ID, card number and device number are used.
 *
 * Can be hashed. Can be compared to another.
 */
struct DeviceKey
{
    QString uniqueId;
    int cardNumber;
    int deviceNumber;

    bool operator==(const DeviceKey &rhs) const;
};

/**
 * \brief Stores information about a device, audio or video
 *
 * The following information is contained in this class:
 * \li Device access list
 * \li Device index
 * \li Preferred name for the user to see
 * \li Description
 * \li Icon
 * \li Initial preference
 * \li Key
 * \li Availability
 * \li Advanced status
 * \li Device number
 *
 * After the device is constructed, the hardware database is searched for overrides.
 *
 * There are methods to allow syncing with a config cache.
 *
 * Can be passed to QDebug().
 *
 * Can be hashed.
 */
class DeviceInfo
{
    public:
        enum Type {
            Unspecified,
            Audio,
            Video
        };

        //! Constructs an empty device info object
        DeviceInfo();

        //! Constructs a device info object from the information given
        DeviceInfo(Type t, const QString &cardName, const QString &icon, const DeviceKey &key,
                int pref, bool adv);

        //! Adds the specified access to the list
        void addAccess(const PS::DeviceAccess &access);

        //! Compare to another device info using the initial device preference
        bool operator<(const DeviceInfo &rhs) const;

        //! Compare if two device info are identical by using the device key
        bool operator==(const DeviceInfo &rhs) const;

        //! Returns the user visible name of the device
        const QString &name() const;

        //! Sets the name of the device that will be visible to the user
        void setPreferredName(const QString &name);

        //! Valid indexes are negative
        int index() const;

        //! User visible string to describe the device in detail
        const QString description() const;

        //! The icon used to portray the device
        const QString &icon() const;

        //! Returns if the device is available for use
        bool isAvailable() const;

        //! Returns if the device is advanced
        bool isAdvanced() const;

        //! The number for the initial preference for the device, before it's changed by the user
        int initialPreference() const;

        //! Number that identifies the device for it's card
        int deviceNumber() const;

        //! A list of accesses for the device, describing how it's possible to access it
        const QList<DeviceAccess> &accessList() const;

        //! Key for the device, to distinguish it from any others
        const DeviceKey &key() const;

        //! Mark the device as deleted, in the specified configuration
        void removeFromCache(const KSharedConfigPtr &config) const;

        //! Write the device information to the specified configuration
        void syncWithCache(const KSharedConfigPtr &config);

    private:
        friend uint qHash(const DeviceInfo &);
        friend QDebug operator<<(QDebug &, const DeviceInfo &);

        void applyHardwareDatabaseOverrides();
        const QString prefixForConfigGroup() const;

    private:
        Type m_type;
        QString m_cardName;
        QString m_icon;

        QList<DeviceAccess> m_accessList;
        DeviceKey m_key;

        int m_index;
        int m_initialPreference;
        bool m_isAvailable : 1;
        bool m_isAdvanced : 1;
        bool m_dbNameOverrideFound : 1;
};

inline QDebug operator<<(QDebug &s, const PS::DeviceKey &k)
{
    s.nospace() << "\n    uniqueId: " << k.uniqueId
        << ", card: " << k.cardNumber
        << ", device: " << k.deviceNumber;
    return s;
}

inline QDebug operator<<(QDebug &s, const PS::DeviceInfo &a)
{
    s.nospace() << "\n- " << a.m_cardName
        << ", icon: " << a.m_icon
        << a.m_key
        << "\n  index: " << a.m_index
        << ", initialPreference: " << a.m_initialPreference
        << ", available: " << a.m_isAvailable
        << ", advanced: " << a.m_isAdvanced
        << ", DB name override: " << a.m_dbNameOverrideFound
        << "\n  description: " << a.description()
        << "\n  access: " << a.m_accessList;
    return s;
}

inline uint qHash(const DeviceKey &k)
{
    return ::qHash(k.uniqueId) + k.cardNumber + 101 * k.deviceNumber;
}

inline uint qHash(const DeviceInfo &a)
{
    return qHash(a.m_key);
}

} // namespace PS


#endif // DEVICEINFO_H

