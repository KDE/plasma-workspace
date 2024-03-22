/*
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include "qwayland-zkde-screencast-unstable-v1.h"

#include <QObject>
#include <QWaylandClientExtensionTemplate>

#include <memory>

class ScreencastingStream : public QObject, public QtWayland::zkde_screencast_stream_unstable_v1
{
    Q_OBJECT

public:
    ScreencastingStream();
    ~ScreencastingStream() override;

Q_SIGNALS:
    void created(quint32 nodeid);
    void failed(const QString &error);
    void closed();

protected:
    void zkde_screencast_stream_unstable_v1_created(uint32_t node) override;
    void zkde_screencast_stream_unstable_v1_closed() override;
    void zkde_screencast_stream_unstable_v1_failed(const QString &error) override;
};

class Screencasting : public QWaylandClientExtensionTemplate<Screencasting>, public QtWayland::zkde_screencast_unstable_v1
{
    Q_OBJECT

public:
    explicit Screencasting();
    ~Screencasting() override;

    std::unique_ptr<ScreencastingStream> createOutputStream(const QString &outputName, pointer mode);
    std::unique_ptr<ScreencastingStream> createWindowStream(const QString &uuid, pointer mode);
};
