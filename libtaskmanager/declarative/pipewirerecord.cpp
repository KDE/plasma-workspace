/*
    SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "pipewirerecord.h"
#include "pipewiresourcestream.h"
#include <epoxy/egl.h>
#include <epoxy/gl.h>

#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QImage>
#include <QMutex>
#include <QThreadPool>
#include <QTimer>

#include <fcntl.h>
#include <unistd.h>

#include <gbm.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/timestamp.h>
#include <libswscale/swscale.h>
}

QByteArray formatGLError(GLenum err)
{
    switch (err) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return QByteArray("0x") + QByteArray::number(err, 16);
    }
}

class CustomAVFrame
{
public:
    CustomAVFrame()
        : m_avFrame(av_frame_alloc())
    {
    }

    ~CustomAVFrame()
    {
        av_freep(&m_avFrame->data[0]);
        av_frame_free(&m_avFrame);
    }

    int alloc(int width, int height, AVPixelFormat pix_fmt)
    {
        m_avFrame->format = pix_fmt;
        m_avFrame->width = width;
        m_avFrame->height = height;
        return av_image_alloc(m_avFrame->data, m_avFrame->linesize, width, height, pix_fmt, 32);
    }

    AVFrame *m_avFrame;
};

PipeWireRecord::PipeWireRecord(QObject *parent)
    : QObject(parent)
{
    av_log_set_level(AV_LOG_DEBUG);
}

PipeWireRecord::~PipeWireRecord()
{
    setActive(false);
}

void PipeWireRecord::setNodeId(uint nodeId)
{
    if (nodeId == m_nodeId)
        return;

    m_nodeId = nodeId;
    refresh();
    Q_EMIT nodeIdChanged(nodeId);
}

void PipeWireRecord::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;
    refresh();
    Q_EMIT activeChanged(active);
}

void PipeWireRecord::setOutput(const QString &output)
{
    if (m_output == output)
        return;

    m_output = output;
    refresh();
    Q_EMIT outputChanged(output);
}

void PipeWireRecordProduce::setupEGL()
{
    if (m_eglInitialized) {
        return;
    }
    m_drmFd = open("/dev/dri/renderD128", O_RDWR);

    if (m_drmFd < 0) {
        qWarning() << "Failed to open drm render node: " << strerror(errno);
        return;
    }

    m_gbmDevice = gbm_create_device(m_drmFd);

    if (!m_gbmDevice) {
        qWarning() << "Cannot create GBM device: " << strerror(errno);
        return;
    }

    // Get the list of client extensions
    const char *clientExtensionsCString = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    const QByteArray clientExtensionsString = QByteArray::fromRawData(clientExtensionsCString, qstrlen(clientExtensionsCString));
    if (clientExtensionsString.isEmpty()) {
        // If eglQueryString() returned NULL, the implementation doesn't support
        // EGL_EXT_client_extensions. Expect an EGL_BAD_DISPLAY error.
        qWarning() << "No client extensions defined! " << formatGLError(eglGetError());
        return;
    }

    m_egl.extensions = clientExtensionsString.split(' ');

    // Use eglGetPlatformDisplayEXT() to get the display pointer
    // if the implementation supports it.
    if (!m_egl.extensions.contains(QByteArrayLiteral("EGL_EXT_platform_base")) || !m_egl.extensions.contains(QByteArrayLiteral("EGL_MESA_platform_gbm"))) {
        qWarning() << "One of required EGL extensions is missing";
        return;
    }

    m_egl.display = eglGetPlatformDisplayEXT(EGL_PLATFORM_GBM_MESA, m_gbmDevice, nullptr);

    if (m_egl.display == EGL_NO_DISPLAY) {
        qWarning() << "Error during obtaining EGL display: " << formatGLError(eglGetError());
        return;
    }

    EGLint major, minor;
    if (eglInitialize(m_egl.display, &major, &minor) == EGL_FALSE) {
        qWarning() << "Error during eglInitialize: " << formatGLError(eglGetError());
        return;
    }

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE) {
        qWarning() << "bind OpenGL API failed";
        return;
    }

    m_egl.context = eglCreateContext(m_egl.display, nullptr, EGL_NO_CONTEXT, nullptr);

    if (m_egl.context == EGL_NO_CONTEXT) {
        qWarning() << "Couldn't create EGL context: " << formatGLError(eglGetError());
        return;
    }

    qDebug() << "Egl initialization succeeded";
    qDebug() << QStringLiteral("EGL version: %1.%2").arg(major).arg(minor);

    m_eglInitialized = true;
}

PipeWireRecordProduce::PipeWireRecordProduce(uint nodeId, const QString &output)
    : QObject()
    , m_output(output)
    , m_nodeId(nodeId)
{
    setupEGL();

    m_stream.reset(new PipeWireSourceStream(nullptr));
    m_stream->createStream(m_nodeId);
    if (!m_stream->error().isEmpty()) {
        qWarning() << "failed to set up stream" << m_stream->error();
        m_stream.reset(nullptr);
        return;
    }
    m_stream->setActive(true);
    connect(m_stream.get(), &PipeWireSourceStream::streamParametersChanged, this, &PipeWireRecordProduce::setupStream);
}

PipeWireRecordProduce::~PipeWireRecordProduce() noexcept
{
    finish();
}

void PipeWireRecordProduceThread::run()
{
    PipeWireRecordProduce produce(m_nodeId, m_output);
    if (!produce.m_stream) {
        return;
    }
    m_producer = &produce;
    qDebug() << "executing";
    const int ret = exec();
    qDebug() << "finishing" << ret;
    m_producer = nullptr;
}

void PipeWireRecordProduceThread::deactivate()
{
    m_producer->m_stream->setActive(false);
}

void PipeWireRecordProduce::finish()
{
    disconnect(m_stream.data(), &PipeWireSourceStream::dmabufTextureReceived, this, &PipeWireRecordProduce::updateTextureDmaBuf);
    disconnect(m_stream.data(), &PipeWireSourceStream::imageTextureReceived, this, &PipeWireRecordProduce::updateTextureImage);
    m_writeThread->drain();
    bool done = QThreadPool::globalInstance()->waitForDone(-1);
    Q_ASSERT(done);

    qDebug() << "finished";
    avio_closep(&m_avFormatContext->pb);
    avcodec_close(m_avCodecContext);
    av_free(m_avCodecContext);
    avformat_free_context(m_avFormatContext);
}

void PipeWireRecordProduce::setupStream()
{
    disconnect(m_stream.get(), &PipeWireSourceStream::streamParametersChanged, this, &PipeWireRecordProduce::setupStream);
    avformat_alloc_output_context2(&m_avFormatContext, NULL, NULL, m_output.toUtf8());
    if (!m_avFormatContext) {
        qWarning() << "Could not deduce output format from file: using MPEG." << m_output;
        avformat_alloc_output_context2(&m_avFormatContext, NULL, "mpeg", m_output.toUtf8());
    }
    if (!m_avFormatContext)
        return;

    // m_codec = avcodec_find_encoder(AV_CODEC_ID_AV1);
    // m_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    // m_codec = avcodec_find_encoder(AV_CODEC_ID_RAWVIDEO);
    // m_codec = avcodec_find_encoder_by_name("libvpx-vp9");
    // m_codec = avcodec_find_encoder_by_name("libx264");
    m_codec = avcodec_find_encoder_by_name("libx264rgb");
    if (!m_codec) {
        qWarning() << "Codec not found";
        return;
    }

    m_avCodecContext = avcodec_alloc_context3(m_codec);
    if (!m_avCodecContext) {
        qWarning() << "Could not allocate video codec context";
        return;
    }
    m_avCodecContext->bit_rate = 500000;

    const QSize size = m_stream->size();
    const Fraction framerate = m_stream->framerate();

    Q_ASSERT(!size.isEmpty());
    m_avCodecContext->width = size.width();
    m_avCodecContext->height = size.height();
    m_avCodecContext->max_b_frames = 1;
    m_avCodecContext->gop_size = 1;
    m_avCodecContext->pix_fmt = m_codec->pix_fmts[0];
    m_avCodecContext->time_base = AVRational{int(framerate.denominator), int(framerate.numerator)};

    av_opt_set(m_avCodecContext->priv_data, "tune-content", "screen", 0);
    av_opt_set_int(m_avCodecContext->priv_data, "cpu-used", 4, 0);
    av_opt_set_int(m_avCodecContext->priv_data, "deadline", 1 /*VPX_DL_REALTIME*/, 0);
    av_opt_set_int(m_avCodecContext->priv_data, "tile-columns", 2, 0);
    av_opt_set_int(m_avCodecContext->priv_data, "tile-rows", 2, 0);
    if (avcodec_open2(m_avCodecContext, m_codec, NULL) < 0) {
        qWarning() << "Could not open codec";
        return;
    }

    m_frame.reset(new CustomAVFrame);
    int ret = m_frame->alloc(m_avCodecContext->width, m_avCodecContext->height, m_avCodecContext->pix_fmt);
    if (ret < 0) {
        qWarning() << "Could not allocate raw picture buffer" << av_err2str(ret);
        return;
    }

    ret = avio_open(&m_avFormatContext->pb, QFile::encodeName(m_output), AVIO_FLAG_WRITE);
    if (ret < 0) {
        qWarning() << "Could not open" << m_output << av_err2str(ret);
        return;
    }

    auto avStream = avformat_new_stream(m_avFormatContext, nullptr);
    avStream->start_time = 0;
    avStream->r_frame_rate.num = framerate.numerator;
    avStream->r_frame_rate.den = framerate.denominator;
    avStream->avg_frame_rate.num = framerate.numerator;
    avStream->avg_frame_rate.den = framerate.denominator;

    ret = avcodec_parameters_from_context(avStream->codecpar, m_avCodecContext);
    if (ret < 0) {
        qWarning() << "Error occurred when passing the codec:" << av_err2str(ret);
        return;
    }

    ret = avformat_write_header(m_avFormatContext, NULL);
    if (ret < 0) {
        qWarning() << "Error occurred when writing header:" << av_err2str(ret);
        return;
    }

    connect(m_stream.data(), &PipeWireSourceStream::dmabufTextureReceived, this, &PipeWireRecordProduce::updateTextureDmaBuf);
    connect(m_stream.data(), &PipeWireSourceStream::imageTextureReceived, this, &PipeWireRecordProduce::updateTextureImage);
    qDebug() << "started";
    m_writeThread = new PipeWireRecordWriteThread(&m_bufferNotEmpty, m_avFormatContext, m_avCodecContext);
    QThreadPool::globalInstance()->start(m_writeThread);
}

void PipeWireRecord::refresh()
{
    if (!m_output.isEmpty() && m_active) {
        m_recordThread = new PipeWireRecordProduceThread(m_nodeId, m_output);
        connect(m_recordThread, &PipeWireRecordProduceThread::finished, this, [this] {
            setActive(false);
        });
        m_recordThread->start();
    } else if (m_recordThread) {
        m_recordThread->deactivate();
        m_recordThread->quit();

        connect(m_recordThread, &PipeWireRecordProduceThread::finished, this, [this] {
            qDebug() << "produce thread finished" << m_output;
            delete m_recordThread;
            m_lastRecordThreadFinished = true;
            Q_EMIT recordingChanged(isRecording());
        });
        m_lastRecordThreadFinished = false;
        m_recordThread = nullptr;
    }
    Q_EMIT recordingChanged(isRecording());
}

void PipeWireRecordProduce::updateTextureDmaBuf(const QVector<DmaBufPlane> &plane, uint32_t format)
{
    Q_ASSERT(qGuiApp->thread() != QThread::currentThread());
    const QSize streamSize = m_stream->size();
    gbm_import_fd_data importInfo = {int(plane[0].fd), uint32_t(streamSize.width()), uint32_t(streamSize.height()), plane[0].stride, GBM_BO_FORMAT_ARGB8888};
    gbm_bo *imported = gbm_bo_import(m_gbmDevice, GBM_BO_IMPORT_FD, &importInfo, GBM_BO_USE_SCANOUT);
    if (!imported) {
        qWarning() << "Failed to process buffer: Cannot import passed GBM fd - " << strerror(errno);
        return;
    }

    // bind context to render thread
    eglMakeCurrent(m_egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_egl.context);

    // create EGL image from imported BO
    EGLImageKHR image = eglCreateImageKHR(m_egl.display, nullptr, EGL_NATIVE_PIXMAP_KHR, imported, nullptr);

    if (image == EGL_NO_IMAGE_KHR) {
        qWarning() << "Failed to record frame: Error creating EGLImageKHR - " << formatGLError(glGetError());
        gbm_bo_destroy(imported);
        return;
    }

    // create GL 2D texture for framebuffer
    GLuint texture;
    glGenTextures(1, &texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, texture);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image);

    auto src = static_cast<uint8_t *>(malloc(plane[0].stride * streamSize.height()));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, src);

    if (!src) {
        qWarning() << "Failed to get image from DMA buffer.";
        gbm_bo_destroy(imported);
        return;
    }

    QImage qimage(src, streamSize.width(), streamSize.height(), plane[0].stride, QImage::Format_ARGB32);
    updateTextureImage(qimage);

    glDeleteTextures(1, &texture);
    eglDestroyImageKHR(m_egl.display, image);

    gbm_bo_destroy(imported);

    free(src);
}

void PipeWireRecordProduce::updateTextureImage(const QImage &image)
{
    QElapsedTimer t;
    t.start();
    const std::uint8_t *buffers[] = {image.constBits(), 0};
    const int strides[] = {image.bytesPerLine(), 0, 0, 0};
    struct SwsContext *sws_context = nullptr;
    sws_context = sws_getCachedContext(sws_context,
                                       image.width(),
                                       image.height(),
                                       AV_PIX_FMT_RGB32,
                                       m_avCodecContext->width,
                                       m_avCodecContext->height,
                                       m_avCodecContext->pix_fmt,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL);
    sws_scale(sws_context, buffers, strides, 0, m_avCodecContext->height, m_frame->m_avFrame->data, m_frame->m_avFrame->linesize);

    if (auto v = m_stream->currentPresentationTimestamp(); v.has_value()) {
        const auto current = std::chrono::duration_cast<std::chrono::milliseconds>(v.value()).count();
        if ((*m_avFormatContext->streams)->start_time == 0) {
            (*m_avFormatContext->streams)->start_time = current;
        }

        Q_ASSERT((*m_avFormatContext->streams)->start_time <= current);
        m_frame->m_avFrame->pts = current - (*m_avFormatContext->streams)->start_time;
    } else {
        m_frame->m_avFrame->pts = AV_NOPTS_VALUE;
    }

    static int i = 0;
    ++i;
    qDebug() << "sending frame" << i << av_ts2str(m_frame->m_avFrame->pts) << "fps: " << double(i * 1000) / double(m_frame->m_avFrame->pts);
    int ret = avcodec_send_frame(m_avCodecContext, m_frame->m_avFrame);
    qDebug() << "sent frames" << i << av_ts2str(m_frame->m_avFrame->pts) << t.elapsed();
    if (ret < 0) {
        qWarning() << "Error sending a frame for encoding:" << av_err2str(ret);
        return;
    }
    m_bufferNotEmpty.wakeAll();
}

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    qDebug("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d",
           av_ts2str(pkt->pts),
           av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts),
           av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration),
           av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

PipeWireRecordWriteThread::PipeWireRecordWriteThread(QWaitCondition *notEmpty, AVFormatContext *avFormatContext, AVCodecContext *avCodecContext)
    : QRunnable()
    , m_packet(av_packet_alloc())
    , m_avFormatContext(avFormatContext)
    , m_avCodecContext(avCodecContext)
    , m_bufferNotEmpty(notEmpty)
{
}

PipeWireRecordWriteThread::~PipeWireRecordWriteThread()
{
    av_packet_free(&m_packet);
}

void PipeWireRecordWriteThread::run()
{
    QMutex mutex;
    int ret = 0;
    while (true) {
        ret = avcodec_receive_packet(m_avCodecContext, m_packet);
        if (ret == AVERROR_EOF) {
            break;
        } else if (ret == AVERROR(EAGAIN)) {
            if (m_active) {
                qDebug() << "{";
                m_bufferNotEmpty->wait(&mutex);
                qDebug() << "}";
            } else {
                qDebug() << "draining" << avcodec_send_frame(m_avCodecContext, NULL);
            }
            continue;
        } else if (ret < 0) {
            qWarning() << "Error encoding a frame: " << av_err2str(ret);
            continue;
        }

        static int i = 0;
        ++i;
        qDebug() << "receiving packets" << i << m_active << av_ts2str(m_packet->pts) << (*m_avFormatContext->streams)->index;
        m_packet->stream_index = (*m_avFormatContext->streams)->index;
        log_packet(m_avFormatContext, m_packet);
        ret = av_interleaved_write_frame(m_avFormatContext, m_packet);
        if (ret < 0) {
            qWarning() << "Error while writing output packet:" << av_err2str(ret);
            continue;
        }
    }
    ret = av_write_trailer(m_avFormatContext);
    if (ret < 0) {
        qWarning() << "failed to write trailer" << av_err2str(ret);
    }
}

void PipeWireRecordWriteThread::drain()
{
    m_active = false;
    m_bufferNotEmpty->wakeAll();
}
