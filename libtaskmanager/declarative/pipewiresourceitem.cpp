/*
    Render a PipeWire stream into a QtQuick scene as a standard Item
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "pipewiresourceitem.h"
#include "logging.h"
#include "pipewiresourcestream.h"

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOpenGLTexture>
#include <QQuickWindow>
#include <QRunnable>
#include <QSGImageNode>
#include <QSocketNotifier>
#include <QThread>
#include <qpa/qplatformnativeinterface.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <libdrm/drm_fourcc.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtPlatformHeaders/QEGLNativeContext>
#endif

static void pwInit()
{
    pw_init(nullptr, nullptr);
}
Q_COREAPP_STARTUP_FUNCTION(pwInit);

class DiscardEglPixmapRunnable : public QRunnable
{
public:
    DiscardEglPixmapRunnable(EGLImageKHR image, QOpenGLTexture *texture)
        : m_image(image)
        , m_texture(texture)
    {
    }

    void run() override
    {
        if (m_image != EGL_NO_IMAGE_KHR) {
            static auto eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
            eglDestroyImageKHR(eglGetCurrentDisplay(), m_image);
        }

        delete m_texture;
    }

private:
    const EGLImageKHR m_image;
    QOpenGLTexture *m_texture;
};

PipeWireSourceItem::PipeWireSourceItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);

    connect(this, &QQuickItem::visibleChanged, this, [this]() {
        setEnabled(isVisible());
        if (m_stream)
            m_stream->setActive(isVisible());
    });
}

PipeWireSourceItem::~PipeWireSourceItem()
{
}

void PipeWireSourceItem::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    switch (change) {
    case ItemVisibleHasChanged:
        setEnabled(isVisible());
        if (m_stream)
            m_stream->setActive(isVisible() && data.boolValue && isComponentComplete());
        break;
    case ItemSceneChange:
        m_needsRecreateTexture = true;
        releaseResources();
        break;
    default:
        break;
    }
}

void PipeWireSourceItem::releaseResources()
{
    if (window()) {
        window()->scheduleRenderJob(new DiscardEglPixmapRunnable(m_image, m_texture.take()), QQuickWindow::NoStage);
        m_image = EGL_NO_IMAGE_KHR;
    }
}

void PipeWireSourceItem::setNodeId(uint nodeId)
{
    if (nodeId == m_nodeId)
        return;

    m_nodeId = nodeId;
    setEnabled(false);

    if (m_nodeId == 0) {
        m_stream.reset(nullptr);
        m_createNextTexture = [] {
            return nullptr;
        };
    } else {
        m_stream.reset(new PipeWireSourceStream(this));
        m_stream->createStream(m_nodeId);
        if (!m_stream->error().isEmpty()) {
            m_stream.reset(nullptr);
            m_nodeId = 0;
            return;
        }
        m_stream->setActive(isVisible() && isComponentComplete());

        connect(m_stream.data(), &PipeWireSourceStream::dmabufTextureReceived, this, &PipeWireSourceItem::updateTextureDmaBuf);
        connect(m_stream.data(), &PipeWireSourceStream::imageTextureReceived, this, &PipeWireSourceItem::updateTextureImage);
    }

    Q_EMIT nodeIdChanged(nodeId);
}

QSGNode *PipeWireSourceItem::updatePaintNode(QSGNode *node, QQuickItem::UpdatePaintNodeData *)
{
    if (Q_UNLIKELY(!m_createNextTexture)) {
        return node;
    }

    auto texture = m_createNextTexture();
    if (!texture) {
        delete node;
        return nullptr;
    }

    if (m_needsRecreateTexture) {
        delete node;
        node = nullptr;
        m_needsRecreateTexture = false;
    }

    QSGImageNode *textureNode = static_cast<QSGImageNode *>(node);
    if (!textureNode) {
        textureNode = window()->createImageNode();
        textureNode->setOwnsTexture(true);
    }
    textureNode->setTexture(texture);

    const auto br = boundingRect().toRect();
    QRect rect({0, 0}, texture->textureSize().scaled(br.size(), Qt::KeepAspectRatio));
    rect.moveCenter(br.center());
    textureNode->setRect(rect);

    return textureNode;
}

QString PipeWireSourceItem::error() const
{
    return m_stream->error();
}

static EGLImage createImage(EGLDisplay display, const QVector<DmaBufPlane> &planes, uint32_t format, const QSize &size)
{
    const bool hasModifiers = planes[0].modifier != DRM_FORMAT_MOD_INVALID;

    QVector<EGLint> attribs;
    attribs << EGL_WIDTH << size.width() << EGL_HEIGHT << size.height() << EGL_LINUX_DRM_FOURCC_EXT << EGLint(format)

            << EGL_DMA_BUF_PLANE0_FD_EXT << planes[0].fd << EGL_DMA_BUF_PLANE0_OFFSET_EXT << EGLint(planes[0].offset) << EGL_DMA_BUF_PLANE0_PITCH_EXT
            << EGLint(planes[0].stride);

    if (hasModifiers) {
        attribs << EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT << EGLint(planes[0].modifier & 0xffffffff) << EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT
                << EGLint(planes[0].modifier >> 32);
    }

    if (planes.count() > 1) {
        attribs << EGL_DMA_BUF_PLANE1_FD_EXT << planes[1].fd << EGL_DMA_BUF_PLANE1_OFFSET_EXT << EGLint(planes[1].offset) << EGL_DMA_BUF_PLANE1_PITCH_EXT
                << EGLint(planes[1].stride);

        if (hasModifiers) {
            attribs << EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT << EGLint(planes[1].modifier & 0xffffffff) << EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT
                    << EGLint(planes[1].modifier >> 32);
        }
    }

    if (planes.count() > 2) {
        attribs << EGL_DMA_BUF_PLANE2_FD_EXT << planes[2].fd << EGL_DMA_BUF_PLANE2_OFFSET_EXT << EGLint(planes[2].offset) << EGL_DMA_BUF_PLANE2_PITCH_EXT
                << EGLint(planes[2].stride);

        if (hasModifiers) {
            attribs << EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT << EGLint(planes[2].modifier & 0xffffffff) << EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT
                    << EGLint(planes[2].modifier >> 32);
        }
    }

    if (planes.count() > 3) {
        attribs << EGL_DMA_BUF_PLANE3_FD_EXT << planes[3].fd << EGL_DMA_BUF_PLANE3_OFFSET_EXT << EGLint(planes[3].offset) << EGL_DMA_BUF_PLANE3_PITCH_EXT
                << EGLint(planes[3].stride);

        if (hasModifiers) {
            attribs << EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT << EGLint(planes[3].modifier & 0xffffffff) << EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT
                    << EGLint(planes[3].modifier >> 32);
        }
    }

    attribs << EGL_NONE;

    static auto eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
    Q_ASSERT(eglCreateImageKHR);

    EGLImage ret = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer) nullptr, attribs.data());
    if (ret == EGL_NO_IMAGE_KHR) {
        qCWarning(PIPEWIRE_LOGGING) << "invalid image" << glGetError();
    }
    //     Q_ASSERT(ret);
    return ret;
}

void PipeWireSourceItem::updateTextureDmaBuf(const QVector<DmaBufPlane> &planes, uint32_t format)
{
    static auto s_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
    if (!s_glEGLImageTargetTexture2DOES) {
        qCWarning(PIPEWIRE_LOGGING) << "glEGLImageTargetTexture2DOES is not available" << window();
        return;
    }

    if (!window()) {
        qCWarning(PIPEWIRE_LOGGING) << "Window not available" << this;
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto openglContext = window()->openglContext();
#else
    const auto openglContext = static_cast<QOpenGLContext *>(window()->rendererInterface()->getResource(window(), QSGRendererInterface::OpenGLContextResource));
#endif
    if (!openglContext || !m_stream) {
        qCWarning(PIPEWIRE_LOGGING) << "need a window and a context" << window();
        return;
    }

    const EGLDisplay display = static_cast<EGLDisplay>(QGuiApplication::platformNativeInterface()->nativeResourceForIntegration("egldisplay"));
    if (m_image) {
        static auto eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
        eglDestroyImageKHR(display, m_image);
    }

    const auto size = m_stream->size();
    m_image = createImage(display, planes, format, size);
    if (m_image == EGL_NO_IMAGE_KHR) {
        QImage img(200, 200, QImage::Format_ARGB32_Premultiplied);
        img.fill(Qt::blue);
        updateTextureImage(img);
        return;
    }

    m_createNextTexture = [this, size, format] {
        if (!m_texture) {
            m_texture.reset(new QOpenGLTexture(QOpenGLTexture::Target2D));
            bool created = m_texture->create();
            Q_ASSERT(created);
        }

        m_texture->bind();

        s_glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)m_image);

        m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
        m_texture->release();
        m_texture->setSize(size.width(), size.height());

        int textureId = m_texture->textureId();
        QQuickWindow::CreateTextureOption textureOption = format == DRM_FORMAT_ARGB8888 ? QQuickWindow::TextureHasAlphaChannel : QQuickWindow::TextureIsOpaque;
        setEnabled(true);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return window()->createTextureFromNativeObject(QQuickWindow::NativeObjectTexture, &textureId, 0 /*a vulkan thing?*/, size, textureOption);
#else
        return QNativeInterface::QSGOpenGLTexture::fromNative(textureId, window(), size, textureOption);
#endif
        ;
    };
    if (window()->isVisible()) {
        update();
    }
}

void PipeWireSourceItem::updateTextureImage(const QImage &image)
{
    if (!window()) {
        qCWarning(PIPEWIRE_LOGGING) << "pass";
        return;
    }

    m_createNextTexture = [this, image] {
        setEnabled(true);
        return window()->createTextureFromImage(image, QQuickWindow::TextureIsOpaque);
    };
    if (window()->isVisible())
        update();
}

void PipeWireSourceItem::componentComplete()
{
    if (m_stream)
        m_stream->setActive(isVisible());
    QQuickItem::componentComplete();
}
