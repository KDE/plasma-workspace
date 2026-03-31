/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "screencasting.h"
#include <QDebug>
#include <QGuiApplication>
#include <QPointer>
#include <QScreen>
#include <qpa/qplatformnativeinterface.h>

ScreencastingStream::ScreencastingStream() = default;

ScreencastingStream::~ScreencastingStream()
{
    destroy();
}

void ScreencastingStream::kde_screencast_stream_v2_created(uint32_t node, uint32_t object_serial_hi, uint32_t object_serial_low)
{
    Q_EMIT created(node, quint64(object_serial_hi) << 32 | object_serial_low);
}

void ScreencastingStream::kde_screencast_stream_v2_closed()
{
    Q_EMIT closed();
}

void ScreencastingStream::kde_screencast_stream_v2_failed(const QString &error)
{
    Q_EMIT failed(error);
}

Screencasting::Screencasting()
    : QWaylandClientExtensionTemplate<Screencasting>(1)
{
    initialize();

    if (!isInitialized()) {
        qWarning() << "Remember to request the interface on your desktop file: X-KDE-Wayland-Interfaces=kde_screencast_v2";
    }
}

Screencasting::~Screencasting()
{
    if (isActive()) {
        destroy();
    }
}

std::unique_ptr<ScreencastingStream> Screencasting::createOutputStream(const QString &outputName, pointer_mode mode)
{
    if (!isActive()) {
        return nullptr;
    }

    QtWayland::kde_screencast_output_params_v2 params(stream_output(outputName));
    params.set_pointer_mode(mode);
    auto stream = std::make_unique<ScreencastingStream>();
    stream->init(params.create_stream());
    return stream;
}

std::unique_ptr<ScreencastingStream> Screencasting::createWindowStream(const QString &uuid, pointer_mode mode)
{
    if (!isActive()) {
        return nullptr;
    }
    QtWayland::kde_screencast_window_params_v2 params(stream_window(uuid));
    params.set_pointer_mode(mode);
    auto stream = std::make_unique<ScreencastingStream>();
    stream->init(params.create_stream());
    return stream;
}

#include "moc_screencasting.cpp"
