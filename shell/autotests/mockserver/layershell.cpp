/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "layershell.h"

namespace MockCompositor
{
LayerShell::LayerShell(CoreCompositor *compositor)
    : QtWaylandServer::zwlr_layer_shell_v1(compositor->m_display, 1)
    , m_compositor(compositor)
{
}

void LayerShell::zwlr_layer_shell_v1_get_layer_surface(Resource *resource,
                                                       uint32_t id,
                                                       struct ::wl_resource *surface_resource,
                                                       struct ::wl_resource *output_resource,
                                                       uint32_t layer,
                                                       const QString &scope)
{
    Surface *surface = resource_cast<Surface *>(surface_resource);
    Output *output = resource_cast<Output *>(output_resource);

    if (layer > layer_overlay) {
        wl_resource_post_error(resource->handle, error_invalid_layer, "invalid layer %d", layer);
        return;
    }

    wl_resource *layerSurfaceResource = wl_resource_create(resource->client(), &zwlr_layer_surface_v1_interface, resource->version(), id);
    if (!layerSurfaceResource) {
        wl_resource_post_no_memory(resource->handle);
        return;
    }

    auto layerSurface = new LayerSurface(this, surface, output, layer, scope, layerSurfaceResource);
    Q_UNUSED(layerSurface)
}

void LayerShell::zwlr_layer_shell_v1_destroy(Resource *resource)
{
    wl_resource_destroy(resource->handle);
}

CoreCompositor *LayerShell::compositor()
{
    return m_compositor;
}

LayerSurface::LayerSurface(LayerShell *shell, Surface *surface, Output *output, uint32_t layer, const QString &scope, wl_resource *resource)
    : QtWaylandServer::zwlr_layer_surface_v1(resource)
    , m_requestedOutput(output)
    , m_layer(layer)
    , m_scope(scope)
    , m_compositor(shell->compositor())
{
    Q_UNUSED(shell)
    surface->m_role = this;
    connect(surface, &Surface::commit, this, [this, surface] {
        m_committed = m_pending;
        qWarning() << "COMMITTED" << m_pending.desiredSize;

        QSize size = m_committed.desiredSize;

        if ((m_committed.anchor & (AnchorLeft | AnchorRight)) == (AnchorLeft | AnchorRight)) {
            size.setWidth(m_requestedOutput->mode().resolution.width());
        }
        if ((m_committed.anchor & (AnchorTop | AnchorBottom)) == (AnchorTop | AnchorBottom)) {
            size.setHeight(m_requestedOutput->mode().resolution.height());
        }

        if (m_pending.pendingSize == size) {
            qWarning() << "SKIPPING";
            return;
        }

        const uint serial = m_compositor->nextSerial();
        m_pending.pendingSize = size;
        qWarning() << "SEND CONFIGURE" << size << serial;
        send_configure(serial, size.width(), size.height());
    });
}

void LayerSurface::zwlr_layer_surface_v1_set_anchor(Resource *resource, uint32_t anchor)
{
    Q_UNUSED(resource);
    m_pending.anchor = anchor;
    qWarning() << "SETTING ANCHOR" << anchor;
}

void LayerSurface::zwlr_layer_surface_v1_set_size(Resource *resource, uint32_t width, uint32_t height)
{
    Q_UNUSED(resource);
    m_pending.desiredSize = QSize(width, height);
    qWarning() << "SETTING DESIRED" << m_pending.desiredSize;
}

} // namespace MockCompositor

#include "moc_layershell.cpp"
