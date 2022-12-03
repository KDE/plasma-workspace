/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencasting.h"
#include "qwayland-zkde-screencast-unstable-v1.h"
#include <QDebug>
#include <QGuiApplication>
#include <QPointer>
#include <QScreen>
#include <QWaylandClientExtensionTemplate>
#include <qpa/qplatformnativeinterface.h>
#include <qtwaylandclientversion.h>

class ScreencastingStreamPrivate : public QtWayland::zkde_screencast_stream_unstable_v1
{
public:
    ScreencastingStreamPrivate(ScreencastingStream *q)
        : q(q)
    {
    }
    ~ScreencastingStreamPrivate()
    {
        close();
        q->deleteLater();
    }

    void zkde_screencast_stream_unstable_v1_created(uint32_t node) override
    {
        m_nodeId = node;
        Q_EMIT q->created(node);
    }

    void zkde_screencast_stream_unstable_v1_closed() override
    {
        Q_EMIT q->closed();
    }

    void zkde_screencast_stream_unstable_v1_failed(const QString &error) override
    {
        Q_EMIT q->failed(error);
    }

    uint m_nodeId = 0;
    QPointer<ScreencastingStream> q;
};

ScreencastingStream::ScreencastingStream(QObject *parent)
    : QObject(parent)
    , d(new ScreencastingStreamPrivate(this))
{
}

ScreencastingStream::~ScreencastingStream() = default;

quint32 ScreencastingStream::nodeId() const
{
    return d->m_nodeId;
}

class ScreencastingPrivate : public QWaylandClientExtensionTemplate<ScreencastingPrivate>, public QtWayland::zkde_screencast_unstable_v1
{
public:
    ScreencastingPrivate(Screencasting *q)
        : QWaylandClientExtensionTemplate<ScreencastingPrivate>(ZKDE_SCREENCAST_UNSTABLE_V1_STREAM_REGION_SINCE_VERSION)
        , q(q)
    {
#if QTWAYLANDCLIENT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
        initialize();
#else
        // QWaylandClientExtensionTemplate invokes this with a QueuedConnection but we want it called immediately
        QMetaObject::invokeMethod(this, "addRegistryListener", Qt::DirectConnection);
#endif

        if (!isInitialized()) {
            qWarning() << "Remember requesting the interface on your desktop file: X-KDE-Wayland-Interfaces=zkde_screencast_unstable_v1";
        }
        Q_ASSERT(isInitialized());
    }

    ~ScreencastingPrivate()
    {
        if (isActive()) {
            destroy();
        }
    }

    Screencasting *const q;
};

Screencasting::Screencasting(QObject *parent)
    : QObject(parent)
    , d(new ScreencastingPrivate(this))
{
}

Screencasting::~Screencasting() = default;

ScreencastingStream *Screencasting::createOutputStream(const QString &outputName, Screencasting::CursorMode mode)
{
    if (!d->isActive()) {
        return nullptr;
    }

    wl_output *output = nullptr;
    for (auto screen : qGuiApp->screens()) {
        if (screen->name() == outputName) {
            output = (wl_output *)QGuiApplication::platformNativeInterface()->nativeResourceForScreen("output", screen);
        }
    }

    if (!output) {
        return nullptr;
    }

    auto stream = new ScreencastingStream(this);
    stream->setObjectName(outputName);
    stream->d->init(d->stream_output(output, mode));
    return stream;
}

ScreencastingStream *Screencasting::createWindowStream(const QString &uuid, CursorMode mode)
{
    if (!d->isActive()) {
        return nullptr;
    }
    auto stream = new ScreencastingStream(this);
    stream->d->init(d->stream_window(uuid, mode));
    return stream;
}

void Screencasting::destroy()
{
    d.reset(nullptr);
}
