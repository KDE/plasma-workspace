/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 or version 3 as published by the Free Software
    Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#ifndef PS_HARDWAREDATABASE_H
#define PS_HARDWAREDATABASE_H

#include <QtCore/QString>

namespace PS
{

/**
 * The HardwareDatabase is used for Phonon to provide more information about devices than are
 * provided through the underlying sound-system.
 * This data is provided through the file `kde4-config --install data`/libphonon/hardwaredatabase.
 * It uses a KConfig parsable format.
 *
 * The implementation here reads in all of that data and converts it to a cache that can be read in
 * much faster, and must only be read where needed.
 *
 * Hardware is identified by a \c udi of the form
 * <bus>:<vendor id>:<product id>:<subsystem vendor id>:<system product id>:<device number>:[playback|capture]
 * Since USB devices don't have the subsystem ids they are not used there.
 */
namespace HardwareDatabase
{
    class Entry;
    class HardwareDatabasePrivate;

    /**
     * Returns whether the hardware database has extra information about the given device.
     */
    bool contains(const QString &udi);

    /**
     * Returns the information in the hardware database for the given device.
     */
    Entry entryFor(const QString &udi);

    class Entry
    {
        public:
            /**
             * The name to display to users.
             */
            const QString name;

            /**
             * Icon to use for this device.
             */
            const QString iconName;

            /**
             * Tells the initial preference in the device list. This determines default ordering of
             * devices and can be used to make sure a default setup uses the correct audio devices.
             */
            const int initialPreference;

            /**
             * Whether this device should be shown as an advanced devices (terrible concept, I
             * know :( )
             */
            const int isAdvanced;

        private:
            friend struct HardwareDatabasePrivate;
            friend Entry entryFor(const QString &);
            inline Entry(const QString &_name, const QString &_iconName, int _initialPreference, int _isAdvanced)
                : name(_name), iconName(_iconName), initialPreference(_initialPreference), isAdvanced(_isAdvanced) {}
            inline Entry() : initialPreference(0), isAdvanced(0) {}
    };
} // namespace HardwareDatabase

} // namespace PS
#endif // PS_HARDWAREDATABASE_H
