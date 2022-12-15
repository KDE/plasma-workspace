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

#include "outputorder.h"

namespace MockCompositor
{
OutputOrder::OutputOrder(CoreCompositor *compositor)
    : QtWaylandServer::kde_output_order_v1(compositor->m_display, 1)
{
}

void OutputOrder::setList(const QStringList &list)
{
    m_list = list;
    const auto resources = resourceMap();
    for (auto *resource : resources) {
        for (const auto &s : std::as_const(list)) {
            send_output(resource->handle, s);
        }
        send_done(resource->handle);
    }
}

void OutputOrder::kde_output_order_v1_bind_resource(Resource *resource)
{
    for (const auto &s : std::as_const(m_list)) {
        send_output(resource->handle, s);
    }
    send_done(resource->handle);
}

} // namespace MockCompositor
