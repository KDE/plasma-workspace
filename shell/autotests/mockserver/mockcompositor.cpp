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

#include "mockcompositor.h"

namespace MockCompositor
{
DefaultCompositor::DefaultCompositor()
{
    {
        Lock l(this);

        add<WlCompositor>();
        add<XdgOutputManagerV1>();
        auto *output = add<Output>();
        auto *outputOrder = add<OutputOrder>();
        output->m_data.physicalSize = output->m_data.mode.physicalSizeForDpi(96);
        add<Seat>(Seat::capability_pointer | Seat::capability_keyboard | Seat::capability_touch);
        add<Shm>();
        add<XdgWmBase>();
        outputOrder->setList({"WL-1"});

        QObject::connect(get<WlCompositor>(), &WlCompositor::surfaceCreated, [&](Surface *surface) {
            QObject::connect(surface, &Surface::bufferCommitted, [=] {
                if (m_config.autoRelease) {
                    // Pretend we made a copy of the buffer and just release it immediately
                    surface->m_committed.buffer->send_release();
                }
                if (m_config.autoEnter && get<Output>() && surface->m_outputs.empty())
                    surface->sendEnter(get<Output>());
                wl_display_flush_clients(m_display);
            });
        });
        QObject::connect(
            get<XdgWmBase>(),
            &XdgWmBase::toplevelCreated,
            get<XdgWmBase>(),
            [&](XdgToplevel *toplevel) {
                if (m_config.autoConfigure)
                    toplevel->sendCompleteConfigure();
            },
            Qt::DirectConnection);
    }
    Q_ASSERT(isClean());
}

uint DefaultCompositor::sendXdgShellPing()
{
    warnIfNotLockedByThread(Q_FUNC_INFO);
    uint serial = nextSerial();
    auto *base = get<XdgWmBase>();
    const auto resourceMap = base->resourceMap();
    Q_ASSERT(resourceMap.size() == 1); // binding more than once shouldn't be needed
    base->send_ping(resourceMap.first()->handle, serial);
    return serial;
}

void DefaultCompositor::xdgPingAndWaitForPong()
{
    QSignalSpy pongSpy(exec([=] {
                           return get<XdgWmBase>();
                       }),
                       &XdgWmBase::pong);
    uint serial = exec([=] {
        return sendXdgShellPing();
    });
    QTRY_COMPARE(pongSpy.count(), 1);
    QTRY_COMPARE(pongSpy.first().at(0).toUInt(), serial);
}

} // namespace MockCompositor
