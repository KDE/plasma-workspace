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

#include <qwayland-server-xdg-output-unstable-v1.h>

namespace MockCompositor
{
class XdgOutputV1 : public QObject, public QtWaylandServer::zxdg_output_v1
{
    Q_OBJECT
public:
    explicit XdgOutputV1(Output *output)
        : m_output(output)
        , m_logicalGeometry(m_output->m_data.position, QSize(m_output->m_data.mode.resolution / m_output->m_data.scale))
        , m_name(m_output->m_data.connector.isEmpty() ? QString("WL-%1").arg(s_nextId++) : m_output->m_data.connector)
    {
    }

    void send_logical_size(int32_t width, int32_t height) = delete;
    void sendLogicalSize(const QSize &size);

    void send_done() = delete; // zxdg_output_v1.done has been deprecated (in protocol version 3)

    void addResource(wl_client *client, int id, int version);
    Output *m_output = nullptr;
    QRect m_logicalGeometry;
    QString m_name;
    QString m_description = "This is an Xdg Output description";
    static int s_nextId;
};

class XdgOutputManagerV1 : public Global, public QtWaylandServer::zxdg_output_manager_v1
{
    Q_OBJECT
public:
    explicit XdgOutputManagerV1(CoreCompositor *compositor)
        : QtWaylandServer::zxdg_output_manager_v1(compositor->m_display, 3)
    {
    }
    QMap<Output *, XdgOutputV1 *> m_xdgOutputs;
    XdgOutputV1 *getXdgOutput(Output *output)
    {
        if (auto *xdgOutput = m_xdgOutputs.value(output)) {
            return xdgOutput;
        }
        m_xdgOutputs[output] = new XdgOutputV1(output);
        connect(output, &Output::destroyed, this, [this, output]() {
            delete m_xdgOutputs[output];
            m_xdgOutputs.remove(output);
        });
        return m_xdgOutputs[output];
    }

protected:
    void zxdg_output_manager_v1_get_xdg_output(Resource *resource, uint32_t id, wl_resource *outputResource) override
    {
        auto *output = fromResource<Output>(outputResource);
        auto *xdgOutput = getXdgOutput(output);
        xdgOutput->addResource(resource->client(), id, resource->version());
    }
};

} // namespace MockCompositor
