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

#include "xdgoutputv1.h"

namespace MockCompositor
{
int XdgOutputV1::s_nextId = 1;

void XdgOutputV1::sendLogicalSize(const QSize &size)
{
    m_logicalGeometry.setSize(size);
    for (auto *resource : resourceMap())
        zxdg_output_v1::send_logical_size(resource->handle, size.width(), size.height());
}

void XdgOutputV1::addResource(wl_client *client, int id, int version)
{
    auto *resource = add(client, id, version)->handle;
    zxdg_output_v1::send_logical_size(resource, m_logicalGeometry.width(), m_logicalGeometry.height());
    send_logical_position(resource, m_logicalGeometry.x(), m_logicalGeometry.y());
    if (version >= ZXDG_OUTPUT_V1_NAME_SINCE_VERSION)
        send_name(resource, m_name);
    if (version >= ZXDG_OUTPUT_V1_DESCRIPTION_SINCE_VERSION)
        send_description(resource, m_description);

    if (version < 3) // zxdg_output_v1.done has been deprecated
        zxdg_output_v1::send_done(resource);
    else {
        m_output->sendDone(client);
    }
}

} // namespace MockCompositor
