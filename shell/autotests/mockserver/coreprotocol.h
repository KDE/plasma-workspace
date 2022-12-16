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

#pragma once

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

class Buffer : public QObject, public QtWaylandServer::wl_buffer
{
    Q_OBJECT
public:
    explicit Buffer(wl_client *client, int id, int version)
        : QtWaylandServer::wl_buffer(client, id, version)
    {
    }
    virtual QSize size() const = 0;
    bool m_destroyed = false;

protected:
    void buffer_destroy_resource(Resource *resource) override
    {
        Q_UNUSED(resource);
        m_destroyed = true;
        // The client side resource has been destroyed, but we keep this object because it may be
        // be used as a reference by e.g. surface for the currently committed buffer so it's not
        // yet safe to free it.

        // TODO: The memory should be freed by its factory
    }
    void buffer_destroy(Resource *resource) override
    {
        wl_resource_destroy(resource->handle);
    }
};

class Callback : public QObject, public QtWaylandServer::wl_callback
{
    Q_OBJECT
public:
    explicit Callback(wl_client *client, int id, int version = 1)
        : QtWaylandServer::wl_callback(client, id, version)
    {
    }
    ~Callback() override
    {
        if (!m_destroyed)
            wl_resource_destroy(resource()->handle);
    }
    void send_done(uint32_t data) = delete; // use state-tracking method below instead
    void sendDone(uint data)
    {
        Q_ASSERT(!m_done);
        QtWaylandServer::wl_callback::send_done(data);
        m_done = true;
    }
    void sendDoneAndDestroy(uint data)
    {
        sendDone(data);
        wl_resource_destroy(resource()->handle);
    }
    bool m_done = false;
    bool m_destroyed = false;

protected:
    void callback_destroy_resource(Resource *resource) override
    {
        Q_UNUSED(resource);
        m_destroyed = true;
    }
};

class SurfaceRole : public QObject
{
    Q_OBJECT
};

class Surface : public QObject, public QtWaylandServer::wl_surface
{
    Q_OBJECT
public:
    explicit Surface(WlCompositor *wlCompositor, wl_client *client, int id, int version)
        : QtWaylandServer::wl_surface(client, id, version)
        , m_wlCompositor(wlCompositor)
    {
    }
    ~Surface() override
    {
        qDeleteAll(m_commits);
    } // TODO: maybe make sure buffers are released?
    void sendFrameCallbacks();
    void sendEnter(Output *output);
    void send_enter(::wl_resource *output) = delete;
    void sendLeave(Output *output);
    void send_leave(::wl_resource *output) = delete;

    WlCompositor *m_wlCompositor;
    struct PerCommitData {
        Callback *frame = nullptr;
        QPoint attachOffset;
        bool attached = false;
    };
    struct DoubleBufferedState {
        PerCommitData commitSpecific;
        Buffer *buffer = nullptr;
        uint configureSerial = 0;
        int bufferScale = 1;
    } m_pending, m_committed;
    QVector<DoubleBufferedState *> m_commits;
    QVector<Callback *> m_waitingFrameCallbacks;
    QVector<Output *> m_outputs;
    SurfaceRole *m_role = nullptr;

signals:
    void attach(void *buffer, QPoint offset);
    void commit();
    void bufferCommitted();

protected:
    void surface_destroy_resource(Resource *resource) override;
    void surface_destroy(Resource *resource) override
    {
        wl_resource_destroy(resource->handle);
    }
    void surface_attach(Resource *resource, wl_resource *buffer, int32_t x, int32_t y) override;
    void surface_set_buffer_scale(Resource *resource, int32_t scale) override;
    void surface_commit(Resource *resource) override;
    void surface_frame(Resource *resource, uint32_t callback) override;
};

class Region : public QtWaylandServer::wl_region
{
public:
    explicit Region(wl_client *client, int id, int version)
        : QtWaylandServer::wl_region(client, id, version)
    {
    }

    void region_destroy_resource(Resource *resource) override
    {
        Q_UNUSED(resource);
        delete this;
    }
};

class WlCompositor : public Global, public QtWaylandServer::wl_compositor
{
    Q_OBJECT
public:
    explicit WlCompositor(CoreCompositor *compositor, int version = 3)
        : QtWaylandServer::wl_compositor(compositor->m_display, version)
        , m_compositor(compositor)
    {
    }
    bool isClean() override;
    QString dirtyMessage() override;
    QVector<Surface *> m_surfaces;
    CoreCompositor *m_compositor = nullptr;

signals:
    void surfaceCreated(Surface *surface);

protected:
    void compositor_create_surface(Resource *resource, uint32_t id) override
    {
        auto *surface = new Surface(this, resource->client(), id, resource->version());
        m_surfaces.append(surface);
        emit surfaceCreated(surface);
    }

    void compositor_create_region(Resource *resource, uint32_t id) override
    {
        new Region(resource->client(), id, resource->version());
    }
};

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
    QString connector; // passing empty string will autogenerate
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

class Seat : public Global, public QtWaylandServer::wl_seat
{
    Q_OBJECT
public:
    explicit Seat(CoreCompositor *compositor,
                  uint capabilities = Seat::capability_pointer | Seat::capability_keyboard | Seat::capability_touch,
                  int version = 5);
    ~Seat() override;
    void send_capabilities(Resource *resource, uint capabilities) = delete; // Use wrapper instead
    void send_capabilities(uint capabilities) = delete; // Use wrapper instead
    void setCapabilities(uint capabilities);

    CoreCompositor *m_compositor = nullptr;

    Pointer *m_pointer = nullptr;
    QVector<Pointer *> m_oldPointers;

    Touch *m_touch = nullptr;
    QVector<Touch *> m_oldTouchs;

    Keyboard *m_keyboard = nullptr;
    QVector<Keyboard *> m_oldKeyboards;

    uint m_capabilities = 0;

protected:
    void seat_bind_resource(Resource *resource) override
    {
        wl_seat::send_capabilities(resource->handle, m_capabilities);
    }

    void seat_get_pointer(Resource *resource, uint32_t id) override;
    void seat_get_touch(Resource *resource, uint32_t id) override;
    void seat_get_keyboard(Resource *resource, uint32_t id) override;

    //    void seat_release(Resource *resource) override;
};

class Pointer : public QObject, public QtWaylandServer::wl_pointer
{
    Q_OBJECT
public:
    explicit Pointer(Seat *seat)
        : m_seat(seat)
    {
    }
    Surface *cursorSurface();
    CursorRole *m_cursorRole = nullptr; // TODO: cleanup
    void send_enter() = delete;
    uint sendEnter(Surface *surface, const QPointF &position);
    void send_leave() = delete;
    uint sendLeave(Surface *surface);
    void sendMotion(wl_client *client, const QPointF &position);
    uint sendButton(wl_client *client, uint button, uint state);
    void sendAxis(wl_client *client, axis axis, qreal value);
    void sendAxisDiscrete(wl_client *client, axis axis, int discrete);
    void sendAxisSource(wl_client *client, axis_source source);
    void sendAxisStop(wl_client *client, axis axis);
    void sendFrame(wl_client *client);

    Seat *m_seat = nullptr;
    QVector<uint> m_enterSerials;
    QPoint m_hotspot;

signals:
    void setCursor(uint serial); // TODO: add arguments?

protected:
    void pointer_set_cursor(Resource *resource, uint32_t serial, ::wl_resource *surface, int32_t hotspot_x, int32_t hotspot_y) override;
    // TODO
};

class CursorRole : public SurfaceRole
{
    Q_OBJECT
public:
    explicit CursorRole(Surface *surface) // TODO: needs some more args
        : m_surface(surface)
    {
    }
    static CursorRole *fromSurface(Surface *surface)
    {
        return qobject_cast<CursorRole *>(surface->m_role);
    }
    Surface *m_surface = nullptr;
};

class Touch : public QObject, public QtWaylandServer::wl_touch
{
    Q_OBJECT
public:
    explicit Touch(Seat *seat)
        : m_seat(seat)
    {
    }
    uint sendDown(Surface *surface, const QPointF &position, int id);
    uint sendUp(wl_client *client, int id);
    void sendMotion(wl_client *client, const QPointF &position, int id);
    void sendFrame(wl_client *client);

    Seat *m_seat = nullptr;
};

class Keyboard : public QObject, public QtWaylandServer::wl_keyboard
{
    Q_OBJECT
public:
    explicit Keyboard(Seat *seat)
        : m_seat(seat)
    {
    }
    // TODO: Keymap event
    uint sendEnter(Surface *surface);
    uint sendLeave(Surface *surface);
    uint sendKey(wl_client *client, uint key, uint state);
    Seat *m_seat = nullptr;
    Surface *m_enteredSurface = nullptr;
};

class Shm : public Global, public QtWaylandServer::wl_shm
{
    Q_OBJECT
public:
    explicit Shm(CoreCompositor *compositor, QVector<format> formats = {format_argb8888, format_xrgb8888, format_rgb888}, int version = 1);
    bool isClean() override;
    CoreCompositor *m_compositor = nullptr;
    QVector<ShmPool *> m_pools;
    const QVector<format> m_formats;

protected:
    void shm_create_pool(Resource *resource, uint32_t id, int32_t fd, int32_t size) override;
    void shm_bind_resource(Resource *resource) override
    {
        for (auto format : qAsConst(m_formats))
            send_format(resource->handle, format);
    }
};

class ShmPool : QObject, public QtWaylandServer::wl_shm_pool
{
    Q_OBJECT
public:
    explicit ShmPool(Shm *shm, wl_client *client, int id, int version = 1);
    Shm *m_shm = nullptr;
    QVector<ShmBuffer *> m_buffers;

protected:
    void shm_pool_create_buffer(Resource *resource, uint32_t id, int32_t offset, int32_t width, int32_t height, int32_t stride, uint32_t format) override;
    void shm_pool_destroy_resource(Resource *resource) override;
    void shm_pool_destroy(Resource *resource) override
    {
        wl_resource_destroy(resource->handle);
    }
};

class ShmBuffer : public Buffer
{
    Q_OBJECT
public:
    static ShmBuffer *fromBuffer(Buffer *buffer)
    {
        return qobject_cast<ShmBuffer *>(buffer);
    }
    explicit ShmBuffer(int offset, const QSize &size, int stride, Shm::format format, wl_client *client, int id, int version = 1)
        : Buffer(client, id, version)
        , m_offset(offset)
        , m_size(size)
        , m_stride(stride)
        , m_format(format)
    {
    }
    QSize size() const override
    {
        return m_size;
    }
    const int m_offset;
    const QSize m_size;
    const int m_stride;
    const Shm::format m_format;
};

} // namespace MockCompositor
