/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "osdwindow.h"
#include "config-X11.h"

#include <PlasmaQuick/PlasmaShellWaylandIntegration>

#if HAVE_X11
#include <KWindowSystem>
#include <KX11Extras>
#endif

OsdWindow::OsdWindow()
{
    PlasmaShellWaylandIntegration::get(this)->setRole(QtWayland::org_kde_plasma_surface::role_onscreendisplay);
    PlasmaShellWaylandIntegration::get(this)->setTakesFocus(false);

    setFlag(Qt::WindowDoesNotAcceptFocus, true);
    setFlag(Qt::WindowTransparentForInput, true);

#if HAVE_X11
    if (KWindowSystem::isPlatformX11()) {
        KX11Extras::setOnAllDesktops(winId(), true);
        KX11Extras::setType(winId(), NET::OnScreenDisplay);
    }
#endif
}
