/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "coreprotocol.h"

namespace MockCompositor
{

void Output::sendGeometry()
{
    const auto resources = resourceMap().values();
    for (auto r : resources)
        sendGeometry(r);
}

void Output::sendGeometry(Resource *resource)
{
    wl_output::send_geometry(resource->handle,
                             m_data.position.x(),
                             m_data.position.y(),
                             m_data.physicalSize.width(),
                             m_data.physicalSize.height(),
                             m_data.subpixel,
                             m_data.make,
                             m_data.model,
                             m_data.transform);
}

void Output::sendScale(int factor)
{
    m_data.scale = factor;
    const auto resources = resourceMap().values();
    for (auto r : resources)
        sendScale(r);
}

void Output::sendScale(Resource *resource)
{
    wl_output::send_scale(resource->handle, m_data.scale);
}

void Output::sendDone(wl_client *client)
{
    auto resources = resourceMap().values(client);
    for (auto *r : resources)
        wl_output::send_done(r->handle);
}

void Output::sendDone()
{
    const auto resources = resourceMap().values();
    for (auto r : resources)
        wl_output::send_done(r->handle);
}

void Output::output_bind_resource(QtWaylandServer::wl_output::Resource *resource)
{
    sendGeometry(resource);
    send_mode(resource->handle, mode_preferred | mode_current, m_data.mode.resolution.width(), m_data.mode.resolution.height(), m_data.mode.refreshRate);
    sendScale(resource);
    wl_output::send_done(resource->handle);
}

} // namespace MockCompositor
