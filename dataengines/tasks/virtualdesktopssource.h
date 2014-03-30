/*
 * Copyright 2010 Matthieu Gallien <matthieu_gallien@yahoo.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef VIRTUALDESKTOPSSOURCE_H
#define VIRTUALDESKTOPSSOURCE_H

// plasma
#include <Plasma/DataContainer>

class VirtualDesktopsSource : public Plasma::DataContainer
{

    Q_OBJECT

    public:

        VirtualDesktopsSource();

        ~VirtualDesktopsSource();

    private Q_SLOTS:

        void updateDesktopNumber(int desktop);

        void updateDesktopNames();
};

#endif
