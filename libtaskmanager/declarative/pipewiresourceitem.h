/*
    Render a PipeWire stream into a QtQuick scene as a standard Item
    SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QQuickItem>
#include <functional>

#include <pipewire/pipewire.h>
#include <spa/param/format-utils.h>
#include <spa/param/props.h>
#include <spa/param/video/format-utils.h>

struct DmaBufPlane;
class PipeWireSourceStream;
class QSGTexture;
class QOpenGLTexture;
typedef void *EGLImage;

class PipeWireSourceItem : public QQuickItem
{
    Q_OBJECT
    /// Specify the pipewire node id that we want to play
    Q_PROPERTY(uint nodeId READ nodeId WRITE setNodeId NOTIFY nodeIdChanged)
public:
    PipeWireSourceItem(QQuickItem *parent = nullptr);
    ~PipeWireSourceItem() override;

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override;
    Q_SCRIPTABLE QString error() const;

    void setNodeId(uint nodeId);
    uint nodeId() const
    {
        return m_nodeId;
    }

    void componentComplete() override;
    void releaseResources() override;

Q_SIGNALS:
    void nodeIdChanged(uint nodeId);

private:
    void itemChange(ItemChange change, const ItemChangeData &data) override;
    void updateTextureDmaBuf(const QVector<DmaBufPlane> &plane, uint32_t format);
    void updateTextureImage(const QImage &image);
    void setSize(const QSize &size);

    uint m_nodeId = 0;
    std::function<QSGTexture *()> m_createNextTexture;
    QScopedPointer<PipeWireSourceStream> m_stream;
    QScopedPointer<QOpenGLTexture> m_texture;

    EGLImage m_image = nullptr;
    bool m_needsRecreateTexture = false;
};
