#pragma once

#include <QSGGeometryNode>
#include <QSGTexture>
#include <QSGTextureMaterial>

class TransientImageNode : public QSGGeometryNode
{
public:
    TransientImageNode();
    ~TransientImageNode() override;

    QSGTexture *texture() const;

    void setTargetRect(const QRectF &rect);
    void setSourceRect(const QRectF &rect);
    void setTexture(std::shared_ptr<QSGTexture> texture);
    void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode);
    void setVerticalWrapMode(QSGTexture::WrapMode wrapMode);
    void setFiltering(QSGTexture::Filtering filtering);

private:
    void rebuildGeometry();

    std::shared_ptr<QSGTexture> m_texture;
    QSGGeometry m_geometry;
    QSGTextureMaterial m_material;
    QSGOpaqueTextureMaterial m_opaqueMaterial;
    QRectF m_targetRect;
    QRectF m_sourceRect;
};
