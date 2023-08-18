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

#pragma once

#include "coreprotocol.h"

#include <qwayland-server-wlr-layer-shell-unstable-v1.h>

namespace MockCompositor
{
class LayerShell : public Global, public QtWaylandServer::zwlr_layer_shell_v1
{
    Q_OBJECT
public:
    explicit LayerShell(CoreCompositor *compositor);

protected:
    void zwlr_layer_shell_v1_get_layer_surface(Resource *resource,
                                               uint32_t id,
                                               struct ::wl_resource *surface,
                                               struct ::wl_resource *output,
                                               uint32_t layer,
                                               const QString &scope) override;
    void zwlr_layer_shell_v1_destroy(Resource *resource) override;

private:
};

class LayerSurface : public SurfaceRole, public QtWaylandServer::zwlr_layer_surface_v1
{
    Q_OBJECT
public:
    explicit LayerSurface(LayerShell *shell, Surface *surface, Output *output, uint32_t layer, const QString &scope, wl_resource *resource);

private:
    Output *m_output = nullptr;
    uint32_t m_layer = 0;
    QString m_scope;
};

} // namespace MockCompositor
