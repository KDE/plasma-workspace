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
    close();
}

void ScreencastingStream::zkde_screencast_stream_unstable_v1_created(uint32_t node)
{
    Q_EMIT created(node);
}

void ScreencastingStream::zkde_screencast_stream_unstable_v1_closed()
{
    Q_EMIT closed();
}

void ScreencastingStream::zkde_screencast_stream_unstable_v1_failed(const QString &error)
{
    Q_EMIT failed(error);
}

void ScreencastingStream::zkde_screencast_stream_unstable_v1_serial(uint32_t object_serial_hi, uint32_t object_serial_low)
{
    Q_EMIT objectSerialArrived(static_cast<quint64>(object_serial_hi) << 32 | object_serial_low);
}

Screencasting::Screencasting()
    : QWaylandClientExtensionTemplate<Screencasting>(ZKDE_SCREENCAST_STREAM_UNSTABLE_V1_SERIAL_SINCE_VERSION)
{
    initialize();

    if (!isInitialized()) {
        qWarning() << "Remember requesting the interface on your desktop file: X-KDE-Wayland-Interfaces=zkde_screencast_unstable_v1";
    }
}

Screencasting::~Screencasting()
{
    if (isActive()) {
        destroy();
    }
}

std::unique_ptr<ScreencastingStream> Screencasting::createOutputStream(const QString &outputName, pointer mode)
{
    if (!isActive()) {
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

    auto stream = std::make_unique<ScreencastingStream>();
    stream->setObjectName(outputName);
    stream->init(stream_output(output, mode));
    return stream;
}

std::unique_ptr<ScreencastingStream> Screencasting::createWindowStream(const QString &uuid, pointer mode)
{
    if (!isActive()) {
        return nullptr;
    }
    auto stream = std::make_unique<ScreencastingStream>();
    stream->init(stream_window(uuid, mode));
    return stream;
}

#include "moc_screencasting.cpp"
