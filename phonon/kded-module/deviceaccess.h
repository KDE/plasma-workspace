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

#ifndef DEVICEACCESS_H
#define DEVICEACCESS_H

#include <QtCore/QDebug>
#include <QtCore/QStringList>

namespace PS
{

/**
 * \brief Describes an access to a device
 *
 * Contains information about the driver, device id, and preference.
 * It also specifies if it is about playback or capture.
 */
class DeviceAccess
{
    public:
        //! What driver to be used for this access
        enum DeviceDriverType {
            InvalidDriver = 0,
            AlsaDriver,
            OssDriver,
            JackdDriver,
            Video4LinuxDriver
        };

        //! Constructs a device access object using the given information
        DeviceAccess(const QStringList &deviceIds, int accessPreference,
                DeviceDriverType driver, bool capture, bool playback);

        //! Compares to another access by using the preference
        bool operator<(const DeviceAccess &rhs) const;

        bool operator==(const DeviceAccess &rhs) const;
        bool operator!=(const DeviceAccess &rhs) const;

        //! Which driver does the access belong to
        DeviceDriverType driver() const;

        /*!
         * Returns the name of the driver
         *
         * If setPreferredDriverName() has not been called with a non-empty name,
         * it returns the name obtained from the driver type.
         */
        const QString driverName() const;

        //! Sets the driver name
        void setPreferredDriverName(const QString &name);

        //! Device identifiers, used to identify the access, driver specific
        const QStringList &deviceIds() const;

        //! Returns the preference for this access
        int accessPreference() const;

        //! True when the access can be used for capture
        bool isCapture() const;

        //! True when the access can be used for playback
        bool isPlayback() const;

    private:
        friend QDebug operator<<(QDebug &, const DeviceAccess &);

        QStringList m_deviceIds;
        int m_accessPreference;
        DeviceDriverType m_driver : 16;
        QString m_preferredName;
        bool m_capture : 8;
        bool m_playback : 8;
};

QDebug operator<<(QDebug &s, const DeviceAccess &a);

} // namespace PS

#endif // DEVICEACCESS_H
