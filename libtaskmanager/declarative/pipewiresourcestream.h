/*
 * Copyright © 2018-2020 Red Hat, Inc
 * Copyright © 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *       Jan Grulich <jgrulich@redhat.com>
 *       Aleix Pol Gonzalez <aleixpol@kde.org>
 */

#pragma once

#include <QObject>
#include <QSize>
#include <QHash>
#include <QSharedPointer>

#include <pipewire/pipewire.h>
#include <spa/param/format-utils.h>
#include <spa/param/video/format-utils.h>
#include <spa/param/props.h>

#undef Status

namespace KWin {
    class AbstractEglBackend;
    class GLTexture;
}
class PipeWireCore;

typedef void *EGLDisplay;

struct DmaBufPlane {
    int fd;             /// The dmabuf file descriptor
    uint32_t offset;    /// The offset from the start of buffer
    uint32_t stride;    /// The distance from the start of a row to the next row in bytes
    uint64_t modifier = 0;  /// The layout modifier
};

class PipeWireSourceStream : public QObject
{
    Q_OBJECT
public:
    explicit PipeWireSourceStream(QObject *parent);
    ~PipeWireSourceStream();

    static void onStreamParamChanged(void *data, uint32_t id, const struct spa_pod *format);
    static void onStreamStateChanged(void *data, pw_stream_state old, pw_stream_state state, const char *error_message);

    uint framerate();
    uint nodeId();
    QString error() const {
        return m_error;
    }

    QSize size() const {
        return QSize(videoFormat.size.width, videoFormat.size.height);
    }
    bool createStream(uint nodeid);
    void stop();
    void setActive(bool active);

    void handleFrame(struct pw_buffer* buffer);
    void process();

Q_SIGNALS:
    void streamReady();
    void startStreaming();
    void stopStreaming();
    void dmabufTextureReceived(const QVector<DmaBufPlane> &planes, uint32_t format);
    void imageTextureReceived(const QImage &image);

private:
    void coreFailed(const QString &errorMessage);

    QSharedPointer<PipeWireCore> pwCore;
    pw_stream *pwStream = nullptr;
    spa_hook streamListener;
    pw_stream_events pwStreamEvents = {};

    uint32_t pwNodeId = 0;

    bool m_stopped = false;

    spa_video_info_raw videoFormat;
    QString m_error;
};
