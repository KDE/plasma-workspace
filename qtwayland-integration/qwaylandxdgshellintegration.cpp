// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandxdgshellintegration_p.h"
#include "qwaylandxdgdecorationv1_p.h"

#include <QtWaylandClient/private/qwaylandwindow_p.h>
#include <QtWaylandClient/private/qwaylanddisplay_p.h>

QT_BEGIN_NAMESPACE

namespace QtWaylandClient {

bool QWaylandXdgShellIntegration::initialize(QWaylandDisplay *display)
{
    for (QWaylandDisplay::RegistryGlobal global : display->globals()) {
        if (global.interface == QLatin1String("xdg_wm_base")) {
            m_xdgShell.reset(new QWaylandXdgShell(display, global.id, global.version));
            break;
        }
    }

    if (!m_xdgShell) {
        qCDebug(lcQpaWayland) << "Couldn't find global xdg_wm_base for xdg-shell stable";
        return false;
    }

    return true;
}

QWaylandShellSurface *QWaylandXdgShellIntegration::createShellSurface(QWaylandWindow *window)
{
    return m_xdgShell->getXdgSurface(window);
}

void *QWaylandXdgShellIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (auto waylandWindow = static_cast<QWaylandWindow *>(window->handle())) {
        if (auto xdgSurface = qobject_cast<QWaylandXdgSurface *>(waylandWindow->shellSurface())) {
            return xdgSurface->nativeResource(resource);
        }
    }
    return nullptr;
}

}

QT_END_NAMESPACE
