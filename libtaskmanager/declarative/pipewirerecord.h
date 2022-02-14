/*
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QFile>
#include <QObject>
#include <QRunnable>
#include <QThread>
#include <QWaitCondition>
#include <functional>

#include <epoxy/egl.h>
#include <pipewire/pipewire.h>
#include <spa/param/format-utils.h>
#include <spa/param/props.h>
#include <spa/param/video/format-utils.h>

class PipeWireSourceStream;
struct DmaBufPlane;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;
struct AVFormatContext;
struct AVPacket;
struct AVStream;
class CustomAVFrame;
struct gbm_device;

class PipeWireRecordWriteThread : public QRunnable
{
public:
    PipeWireRecordWriteThread(QWaitCondition *notEmpty, AVFormatContext *avFormatContext, AVCodecContext *avCodecContext);
    ~PipeWireRecordWriteThread();

    void run() override;
    void drain();

private:
    QAtomicInt m_active = true;
    AVPacket *m_packet;
    AVFormatContext *const m_avFormatContext;
    AVCodecContext *const m_avCodecContext;
    QWaitCondition *const m_bufferNotEmpty;
};

class PipeWireRecordProduce : public QObject
{
public:
    PipeWireRecordProduce(uint nodeId, const QString &output);
    ~PipeWireRecordProduce() override;

    void finish();

private:
    friend class PipeWireRecordProduceThread;
    void setupEGL();
    void setupStream();
    void updateTextureDmaBuf(const QVector<DmaBufPlane> &plane, uint32_t format);
    void updateTextureImage(const QImage &image);

    AVCodecContext *m_avCodecContext = nullptr;
    const AVCodec *m_codec = nullptr;
    AVFormatContext *m_avFormatContext = nullptr;
    const QString m_output;
    const uint m_nodeId;
    QScopedPointer<PipeWireSourceStream> m_stream;

    struct EGLStruct {
        QList<QByteArray> extensions;
        EGLDisplay display = EGL_NO_DISPLAY;
        EGLContext context = EGL_NO_CONTEXT;
    };

    bool m_eglInitialized = false;
    qint32 m_drmFd = 0; // for GBM buffer mmap
    gbm_device *m_gbmDevice = nullptr; // for passed GBM buffer retrieval

    EGLStruct m_egl;
    PipeWireRecordWriteThread *m_writeThread;
    QWaitCondition m_bufferNotEmpty;

    QScopedPointer<CustomAVFrame> m_frame;
};

class PipeWireRecordProduceThread : public QThread
{
public:
    PipeWireRecordProduceThread(uint nodeId, const QString &output)
        : m_nodeId(nodeId)
        , m_output(output)
    {
    }
    void run() override;
    void deactivate();

private:
    const uint m_nodeId;
    const QString m_output;
    PipeWireRecordProduce *m_producer = nullptr;
};

class PipeWireRecord : public QObject
{
    Q_OBJECT
    /// Specify the pipewire node id that we want to record
    Q_PROPERTY(uint nodeId READ nodeId WRITE setNodeId NOTIFY nodeIdChanged)
    Q_PROPERTY(bool active READ isActive WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool recording READ isRecording NOTIFY recordingChanged)
    Q_PROPERTY(QString output READ output WRITE setOutput NOTIFY outputChanged)
public:
    PipeWireRecord(QObject *parent = nullptr);
    ~PipeWireRecord() override;

    void setNodeId(uint nodeId);
    uint nodeId() const
    {
        return m_nodeId;
    }

    bool isRecording() const
    {
        return m_recordThread || !m_lastRecordThreadFinished;
    }
    bool isActive() const
    {
        return m_active;
    }
    void setActive(bool active);

    QString output() const
    {
        return m_output;
    }
    void setOutput(const QString &output);

Q_SIGNALS:
    void activeChanged(bool active);
    void recordingChanged(bool recording);
    void nodeIdChanged(uint nodeId);
    void outputChanged(const QString &output);

private:
    void refresh();

    uint m_nodeId = 0;
    bool m_active = false;
    QString m_output;
    PipeWireRecordProduceThread *m_recordThread = nullptr;
    bool m_lastRecordThreadFinished = true;
};
