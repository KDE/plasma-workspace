/*
    SPDX-FileCopyrightText: 2026 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "osdwindow.h"

#include <PlasmaQuick/PlasmaShellWaylandIntegration>

OsdWindow::OsdWindow()
{
    PlasmaShellWaylandIntegration::get(this)->setRole(QtWayland::org_kde_plasma_surface::role_onscreendisplay);
    PlasmaShellWaylandIntegration::get(this)->setTakesFocus(false);

    setFlag(Qt::WindowDoesNotAcceptFocus, true);
    setFlag(Qt::WindowTransparentForInput, true);
}
