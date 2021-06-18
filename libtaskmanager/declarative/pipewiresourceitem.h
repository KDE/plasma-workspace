/*
 * Render a PipeWire stream into a QtQuick scene as a standard Item
 * Copyright 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <https://www.gnu.org/licenses/>.
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
