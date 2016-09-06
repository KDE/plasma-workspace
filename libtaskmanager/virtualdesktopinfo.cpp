/********************************************************************
Copyright 2016  Eike Hein <hein.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) version 3, or any
later version accepted by the membership of KDE e.V. (or its
successor approved by the membership of KDE e.V.), which shall
act as a proxy defined in Section 6 of version 3 of the license.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************/

#include "virtualdesktopinfo.h"

#include <KWindowSystem>

#include <QDBusConnection>

#include <config-X11.h>

#if HAVE_X11
#include <netwm.h>
#include <QX11Info>
#endif

namespace TaskManager
{

VirtualDesktopInfo::VirtualDesktopInfo(QObject *parent) : QObject(parent)
{
    connect(KWindowSystem::self(), &KWindowSystem::currentDesktopChanged,
            this, &VirtualDesktopInfo::currentDesktopChanged);

    connect(KWindowSystem::self(), &KWindowSystem::numberOfDesktopsChanged,
            this, &VirtualDesktopInfo::numberOfDesktopsChanged);

    connect(KWindowSystem::self(), &KWindowSystem::desktopNamesChanged,
            this, &VirtualDesktopInfo::desktopNamesChanged);

#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        QDBusConnection dbus = QDBusConnection::sessionBus();
        dbus.connect(QString(), QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin"), QStringLiteral("reloadConfig"),
                    this, SIGNAL(desktopLayoutRowsChanged()));
    }
#endif
}

VirtualDesktopInfo::~VirtualDesktopInfo()
{
}

int VirtualDesktopInfo::currentDesktop() const
{
    return KWindowSystem::currentDesktop();
}

int VirtualDesktopInfo::numberOfDesktops() const
{
    return KWindowSystem::numberOfDesktops();
}

QStringList VirtualDesktopInfo::desktopNames() const
{
    QStringList names;

    // Virtual desktop numbers start at 1.
    for (int i = 1; i <= KWindowSystem::numberOfDesktops(); ++i) {
        names << KWindowSystem::desktopName(i);
    }

    return names;
}

int VirtualDesktopInfo::desktopLayoutRows() const
{
#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        const NETRootInfo info(QX11Info::connection(), NET::NumberOfDesktops | NET::DesktopNames, NET::WM2DesktopLayout);
        return info.desktopLayoutColumnsRows().height();
    }
#endif

    return 0;
}

}
