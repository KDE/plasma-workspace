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

#ifndef MOCKCOMPOSITOR_COREPROTOCOL_H
#define MOCKCOMPOSITOR_COREPROTOCOL_H

#include "corecompositor.h"

#include <qwayland-server-wayland.h>

namespace MockCompositor
{
class WlCompositor;
class Output;
class Pointer;
class Touch;
class Keyboard;
class CursorRole;
class ShmPool;
class ShmBuffer;
class DataDevice;

struct OutputMode {
    explicit OutputMode() = default;
    explicit OutputMode(const QSize &resolution, int refreshRate = 60000)
        : resolution(resolution)
        , refreshRate(refreshRate)
    {
    }
    QSize resolution = QSize(1920, 1080);
    int refreshRate = 60000; // In mHz
    // TODO: flags (they're currently hard-coded)

    // in mm
    QSize physicalSizeForDpi(int dpi)
    {
        return (QSizeF(resolution) * 25.4 / dpi).toSize();
    }
};

struct OutputData {
    using Subpixel = QtWaylandServer::wl_output::subpixel;
    using Transform = QtWaylandServer::wl_output::transform;
    explicit OutputData() = default;

    // for geometry event
    QPoint position;
    QSize physicalSize = QSize(0, 0); // means unknown physical size
    QString make = "Make";
    QString model = "Model";
    Subpixel subpixel = Subpixel::subpixel_unknown;
    Transform transform = Transform::transform_normal;

    int scale = 1; // for scale event
    OutputMode mode; // for mode event
};

class Output : public Global, public QtWaylandServer::wl_output
{
    Q_OBJECT
public:
    explicit Output(CoreCompositor *compositor, OutputData data = OutputData())
        : QtWaylandServer::wl_output(compositor->m_display, 2)
        , m_data(std::move(data))
    {
    }

    void send_geometry() = delete;
    void sendGeometry();
    void sendGeometry(Resource *resource); // Sends to only one client

    void send_scale(int32_t factor) = delete;
    void sendScale(int factor);
    void sendScale(Resource *resource); // Sends current scale to only one client

    void sendDone(wl_client *client);
    void sendDone();

    int scale() const
    {
        return m_data.scale;
    }

    OutputData m_data;

protected:
    void output_bind_resource(Resource *resource) override;
};

} // namespace MockCompositor

#endif // MOCKCOMPOSITOR_COREPROTOCOL_H
