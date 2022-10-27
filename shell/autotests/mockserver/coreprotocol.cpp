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
#include "datadevice.h"

namespace MockCompositor
{
void Surface::sendFrameCallbacks()
{
    uint time = m_wlCompositor->m_compositor->currentTimeMilliseconds();
    for (auto *callback : m_waitingFrameCallbacks)
        callback->sendDone(time);
    m_waitingFrameCallbacks.clear();
}

void Surface::sendEnter(Output *output)
{
    m_outputs.append(output);
    const auto outputResources = output->resourceMap().values(resource()->client());
    for (auto outputResource : outputResources)
        wl_surface::send_enter(resource()->handle, outputResource->handle);
}

void Surface::sendLeave(Output *output)
{
    m_outputs.removeOne(output);
    const auto outputResources = output->resourceMap().values(resource()->client());
    for (auto outputResource : outputResources)
        wl_surface::send_leave(resource()->handle, outputResource->handle);
}

void Surface::surface_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    for (auto *commit : m_commits)
        delete commit->commitSpecific.frame;
    bool removed = m_wlCompositor->m_surfaces.removeOne(this);
    Q_ASSERT(removed);
    delete this;
}

void Surface::surface_attach(Resource *resource, wl_resource *buffer, int32_t x, int32_t y)
{
    Q_UNUSED(resource);
    QPoint offset(x, y);
    m_pending.buffer = fromResource<Buffer>(buffer);
    m_pending.commitSpecific.attachOffset = offset;
    m_pending.commitSpecific.attached = true;
    emit attach(buffer, offset);
}

void Surface::surface_set_buffer_scale(QtWaylandServer::wl_surface::Resource *resource, int32_t scale)
{
    Q_UNUSED(resource);
    m_pending.bufferScale = scale;
}

void Surface::surface_commit(Resource *resource)
{
    Q_UNUSED(resource);
    m_committed = m_pending;
    m_commits.append(new DoubleBufferedState(m_committed));

    if (auto *frame = m_pending.commitSpecific.frame)
        m_waitingFrameCallbacks.append(frame);

    m_pending.commitSpecific = PerCommitData();
    emit commit();
    if (m_committed.commitSpecific.attached)
        emit bufferCommitted();
}

void Surface::surface_frame(Resource *resource, uint32_t callback)
{
    // Although valid, there is really no point having multiple frame requests in the same commit.
    // Make sure we don't do it
    QCOMPARE(m_pending.commitSpecific.frame, nullptr);

    auto *frame = new Callback(resource->client(), callback, 1);
    m_pending.commitSpecific.frame = frame;
}

bool WlCompositor::isClean()
{
    for (auto *surface : qAsConst(m_surfaces)) {
        if (!CursorRole::fromSurface(surface))
            return false;
    }
    return true;
}

QString WlCompositor::dirtyMessage()
{
    if (isClean())
        return "clean";
    QStringList messages;
    for (auto *s : qAsConst(m_surfaces)) {
        QString role = s->m_role ? s->m_role->staticMetaObject.className() : "none/unknown";
        messages << "Surface with role: " + role;
    }
    return "Dirty, surfaces left:\n\t" + messages.join("\n\t");
}

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

// Seat stuff
Seat::Seat(CoreCompositor *compositor, uint capabilities, int version) // TODO: check version
    : QtWaylandServer::wl_seat(compositor->m_display, version)
    , m_compositor(compositor)
{
    setCapabilities(capabilities);
}

Seat::~Seat()
{
    qDeleteAll(m_oldPointers);
    delete m_pointer;

    qDeleteAll(m_oldTouchs);
    delete m_touch;

    qDeleteAll(m_oldKeyboards);
    delete m_keyboard;
}

void Seat::setCapabilities(uint capabilities)
{
    m_capabilities = capabilities;

    if (m_capabilities & capability_pointer) {
        if (!m_pointer)
            m_pointer = (new Pointer(this));
    } else if (m_pointer) {
        m_oldPointers << m_pointer;
        m_pointer = nullptr;
    }

    if (m_capabilities & capability_touch) {
        if (!m_touch)
            m_touch = (new Touch(this));
    } else if (m_touch) {
        m_oldTouchs << m_touch;
        m_touch = nullptr;
    }

    if (m_capabilities & capability_keyboard) {
        if (!m_keyboard)
            m_keyboard = (new Keyboard(this));
    } else if (m_keyboard) {
        m_oldKeyboards << m_keyboard;
        m_keyboard = nullptr;
    }

    for (auto *resource : resourceMap())
        wl_seat::send_capabilities(resource->handle, capabilities);
}

void Seat::seat_get_pointer(Resource *resource, uint32_t id)
{
    if (~m_capabilities & capability_pointer) {
        qWarning() << "Client requested a wl_pointer without the capability being available."
                   << "This Could be a race condition when hotunplugging,"
                   << "but is most likely a client error";
        Pointer *pointer = new Pointer(this);
        pointer->add(resource->client(), id, resource->version());
        // TODO: mark as destroyed
        m_oldPointers << pointer;
        return;
    }
    m_pointer->add(resource->client(), id, resource->version());
}

void Seat::seat_get_touch(QtWaylandServer::wl_seat::Resource *resource, uint32_t id)
{
    if (~m_capabilities & capability_touch) {
        qWarning() << "Client requested a wl_touch without the capability being available."
                   << "This Could be a race condition when hotunplugging,"
                   << "but is most likely a client error";
        Touch *touch = new Touch(this);
        touch->add(resource->client(), id, resource->version());
        // TODO: mark as destroyed
        m_oldTouchs << touch;
        return;
    }
    m_touch->add(resource->client(), id, resource->version());
}

void Seat::seat_get_keyboard(QtWaylandServer::wl_seat::Resource *resource, uint32_t id)
{
    if (~m_capabilities & capability_keyboard) {
        qWarning() << "Client requested a wl_keyboard without the capability being available."
                   << "This Could be a race condition when hotunplugging,"
                   << "but is most likely a client error";
        Keyboard *keyboard = new Keyboard(this);
        keyboard->add(resource->client(), id, resource->version());
        // TODO: mark as destroyed
        m_oldKeyboards << keyboard;
        return;
    }
    m_keyboard->add(resource->client(), id, resource->version());
}

Surface *Pointer::cursorSurface()
{
    return m_cursorRole ? m_cursorRole->m_surface : nullptr;
}

uint Pointer::sendEnter(Surface *surface, const QPointF &position)
{
    wl_fixed_t x = wl_fixed_from_double(position.x());
    wl_fixed_t y = wl_fixed_from_double(position.y());

    uint serial = m_seat->m_compositor->nextSerial();
    m_enterSerials << serial;
    m_cursorRole = nullptr; // According to the protocol, the pointer image is undefined after enter

    wl_client *client = surface->resource()->client();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        wl_pointer::send_enter(r->handle, serial, surface->resource()->handle, x, y);
    return serial;
}

uint Pointer::sendLeave(Surface *surface)
{
    uint serial = m_seat->m_compositor->nextSerial();

    wl_client *client = surface->resource()->client();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        wl_pointer::send_leave(r->handle, serial, surface->resource()->handle);
    return serial;
}

// Make sure you call enter, frame etc. first
void Pointer::sendMotion(wl_client *client, const QPointF &position)
{
    wl_fixed_t x = wl_fixed_from_double(position.x());
    wl_fixed_t y = wl_fixed_from_double(position.y());
    auto time = m_seat->m_compositor->currentTimeMilliseconds();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_motion(r->handle, time, x, y);
}

// Make sure you call enter, frame etc. first
uint Pointer::sendButton(wl_client *client, uint button, uint state)
{
    Q_ASSERT(state == button_state_pressed || state == button_state_released);
    auto time = m_seat->m_compositor->currentTimeMilliseconds();
    uint serial = m_seat->m_compositor->nextSerial();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_button(r->handle, serial, time, button, state);
    return serial;
}

// Make sure you call enter, frame etc. first
void Pointer::sendAxis(wl_client *client, axis axis, qreal value)
{
    auto time = m_seat->m_compositor->currentTimeMilliseconds();
    wl_fixed_t val = wl_fixed_from_double(value);
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_axis(r->handle, time, axis, val);
}

void Pointer::sendAxisDiscrete(wl_client *client, QtWaylandServer::wl_pointer::axis axis, int discrete)
{
    // TODO: assert v5 or newer
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_axis_discrete(r->handle, axis, discrete);
}

void Pointer::sendAxisSource(wl_client *client, QtWaylandServer::wl_pointer::axis_source source)
{
    // TODO: assert v5 or newer
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_axis_source(r->handle, source);
}

void Pointer::sendAxisStop(wl_client *client, QtWaylandServer::wl_pointer::axis axis)
{
    // TODO: assert v5 or newer
    auto time = m_seat->m_compositor->currentTimeMilliseconds();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_axis_stop(r->handle, time, axis);
}

void Pointer::sendFrame(wl_client *client)
{
    // TODO: assert version 5 or newer?
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_frame(r->handle);
}

void Pointer::pointer_set_cursor(Resource *resource, uint32_t serial, wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y)
{
    Q_UNUSED(resource);
    auto *s = fromResource<Surface>(surface);
    QVERIFY(s);

    if (s->m_role) {
        m_cursorRole = CursorRole::fromSurface(s);
        QVERIFY(m_cursorRole);
    } else {
        m_cursorRole = new CursorRole(s); // TODO: make sure we don't leak CursorRole
        s->m_role = m_cursorRole;
    }

    // Directly checking the last serial would be racy, we may just have sent leaves/enters which
    // the client hasn't yet seen. Instead just check if the serial matches an enter serial since
    // the last time the client sent a set_cursor request.
    QVERIFY(m_enterSerials.contains(serial));
    while (m_enterSerials.first() < serial) {
        m_enterSerials.removeFirst();
    }

    m_hotspot = QPoint(hotspot_x, hotspot_y);
    emit setCursor(serial);
}

uint Touch::sendDown(Surface *surface, const QPointF &position, int id)
{
    wl_fixed_t x = wl_fixed_from_double(position.x());
    wl_fixed_t y = wl_fixed_from_double(position.y());
    uint serial = m_seat->m_compositor->nextSerial();
    auto time = m_seat->m_compositor->currentTimeMilliseconds();
    wl_client *client = surface->resource()->client();

    const auto touchResources = resourceMap().values(client);
    for (auto *r : touchResources)
        wl_touch::send_down(r->handle, serial, time, surface->resource()->handle, id, x, y);

    return serial;
}

uint Touch::sendUp(wl_client *client, int id)
{
    uint serial = m_seat->m_compositor->nextSerial();
    auto time = m_seat->m_compositor->currentTimeMilliseconds();

    const auto touchResources = resourceMap().values(client);
    for (auto *r : touchResources)
        wl_touch::send_up(r->handle, serial, time, id);

    return serial;
}

void Touch::sendMotion(wl_client *client, const QPointF &position, int id)
{
    wl_fixed_t x = wl_fixed_from_double(position.x());
    wl_fixed_t y = wl_fixed_from_double(position.y());

    auto time = m_seat->m_compositor->currentTimeMilliseconds();

    const auto touchResources = resourceMap().values(client);
    for (auto *r : touchResources)
        wl_touch::send_motion(r->handle, time, id, x, y);
}

void Touch::sendFrame(wl_client *client)
{
    const auto touchResources = resourceMap().values(client);
    for (auto *r : touchResources)
        send_frame(r->handle);
}

uint Keyboard::sendEnter(Surface *surface)
{
    auto serial = m_seat->m_compositor->nextSerial();
    wl_client *client = surface->resource()->client();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_enter(r->handle, serial, surface->resource()->handle, QByteArray());
    m_enteredSurface = surface;
    return serial;
}

uint Keyboard::sendLeave(Surface *surface)
{
    auto serial = m_seat->m_compositor->nextSerial();
    wl_client *client = surface->resource()->client();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_leave(r->handle, serial, surface->resource()->handle);
    m_enteredSurface = nullptr;
    return serial;
}

uint Keyboard::sendKey(wl_client *client, uint key, uint state)
{
    Q_ASSERT(state == key_state_pressed || state == key_state_released);
    auto time = m_seat->m_compositor->currentTimeMilliseconds();
    uint serial = m_seat->m_compositor->nextSerial();
    const auto pointerResources = resourceMap().values(client);
    for (auto *r : pointerResources)
        send_key(r->handle, serial, time, key, state);
    return serial;
}

// Shm implementation
Shm::Shm(CoreCompositor *compositor, QVector<format> formats, int version)
    : QtWaylandServer::wl_shm(compositor->m_display, version)
    , m_compositor(compositor)
    , m_formats(formats)
{
    // Some formats are specified as mandatory
    Q_ASSERT(m_formats.contains(format_argb8888));
    Q_ASSERT(m_formats.contains(format_xrgb8888));
}

bool Shm::isClean()
{
    //    for (ShmPool *pool : qAsConst(m_pools)) {
    //        //TODO: return false if not cursor buffer
    //        if (pool->m_buffers.isEmpty()) {
    //            return false;
    //        }
    //    }
    return true;
}

void Shm::shm_create_pool(Resource *resource, uint32_t id, int32_t fd, int32_t size)
{
    Q_UNUSED(fd);
    Q_UNUSED(size);
    auto *pool = new ShmPool(this, resource->client(), id, 1);
    m_pools.append(pool);
}

ShmPool::ShmPool(Shm *shm, wl_client *client, int id, int version)
    : QtWaylandServer::wl_shm_pool(client, id, version)
    , m_shm(shm)
{
}

void ShmPool::shm_pool_create_buffer(Resource *resource, uint32_t id, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format)
{
    QSize size(width, height);
    new ShmBuffer(offset, size, stride, Shm::format(format), resource->client(), id);
}

void ShmPool::shm_pool_destroy_resource(Resource *resource)
{
    Q_UNUSED(resource);
    bool removed = m_shm->m_pools.removeOne(this);
    Q_ASSERT(removed);
    delete this;
}

} // namespace MockCompositor
