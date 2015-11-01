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

#include "virtualdesktopssource.h"

#include <KWindowSystem>

VirtualDesktopsSource::VirtualDesktopsSource() : Plasma::DataContainer()
{
    setObjectName( QLatin1String("virtualDesktops" ));
    connect(KWindowSystem::self(), &KWindowSystem::numberOfDesktopsChanged, this, &VirtualDesktopsSource::updateDesktopNumber);
    connect(KWindowSystem::self(), &KWindowSystem::desktopNamesChanged, this, &VirtualDesktopsSource::updateDesktopNames);
    updateDesktopNumber(KWindowSystem::self()->numberOfDesktops());
    updateDesktopNames();
}

VirtualDesktopsSource::~VirtualDesktopsSource()
{
    disconnect(KWindowSystem::self(), &KWindowSystem::numberOfDesktopsChanged, this, &VirtualDesktopsSource::updateDesktopNumber);
}

void VirtualDesktopsSource::updateDesktopNumber(int desktop)
{
    setData(QStringLiteral("number"), desktop);
    checkForUpdate();
}

void VirtualDesktopsSource::updateDesktopNames()
{
    QList<QVariant> desktopNames;
    for (int i = 0; i < KWindowSystem::self()->numberOfDesktops(); i++) {
        desktopNames.append(KWindowSystem::self()->desktopName(i + 1));
    }
    setData(QStringLiteral("names"), desktopNames);
    checkForUpdate();
}
