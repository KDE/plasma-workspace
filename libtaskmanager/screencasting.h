/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "qwayland-kde-screencast-v2.h"

#include <QObject>
#include <QWaylandClientExtensionTemplate>
#include <qqmlregistration.h>

#include <memory>

class ScreencastingStream : public QObject, public QtWayland::kde_screencast_stream_v2
{
    Q_OBJECT

public:
    ScreencastingStream();
    ~ScreencastingStream() override;

Q_SIGNALS:
    void created(quint32 nodeid, quint64 objectSerial);
    void failed(const QString &error);
    void closed();

protected:
    void kde_screencast_stream_v2_created(uint32_t node, uint32_t object_serial_hi, uint32_t object_serial_low) override;
    void kde_screencast_stream_v2_closed() override;
    void kde_screencast_stream_v2_failed(const QString &error) override;
};

class Screencasting : public QWaylandClientExtensionTemplate<Screencasting>, public QtWayland::kde_screencast_manager_v2
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Use ScreencastingItem")

public:
    explicit Screencasting();
    ~Screencasting() override;

    std::unique_ptr<ScreencastingStream> createOutputStream(const QString &outputName, pointer_mode mode);
    std::unique_ptr<ScreencastingStream> createWindowStream(const QString &uuid, pointer_mode mode);
};
