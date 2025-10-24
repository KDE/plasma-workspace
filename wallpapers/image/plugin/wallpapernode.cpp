#include "wallpapernode.h"

TransientImageNode::TransientImageNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    setGeometry(&m_geometry);
    setMaterial(&m_material);
    setOpaqueMaterial(&m_opaqueMaterial);
}

TransientImageNode::~TransientImageNode()
{
}

QSGTexture *TransientImageNode::texture() const
{
    return m_material.texture();
}

void TransientImageNode::setTargetRect(const QRectF &rect)
{
    if (m_targetRect == rect) {
        return;
    }

    m_targetRect = rect;
    rebuildGeometry();
    markDirty(QSGNode::DirtyGeometry);
}

void TransientImageNode::setSourceRect(const QRectF &rect)
{
    if (m_sourceRect == rect) {
        return;
    }

    m_sourceRect = rect;
    rebuildGeometry();
    markDirty(QSGNode::DirtyGeometry);
}

void TransientImageNode::setTexture(std::shared_ptr<QSGTexture> texture)
{
    if (m_texture == texture) {
        return;
    }

    m_texture = texture;

    m_material.setTexture(texture.get());
    m_opaqueMaterial.setTexture(texture.get());

    rebuildGeometry();
    markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
}

void TransientImageNode::setHorizontalWrapMode(QSGTexture::WrapMode wrapMode)
{
    if (m_material.horizontalWrapMode() == wrapMode) {
        return;
    }

    m_material.setHorizontalWrapMode(wrapMode);
    m_opaqueMaterial.setHorizontalWrapMode(wrapMode);
    markDirty(QSGNode::DirtyMaterial);
}

void TransientImageNode::setVerticalWrapMode(QSGTexture::WrapMode wrapMode)
{
    if (m_material.verticalWrapMode() == wrapMode) {
        return;
    }

    m_material.setVerticalWrapMode(wrapMode);
    m_opaqueMaterial.setVerticalWrapMode(wrapMode);
    markDirty(QSGNode::DirtyMaterial);
}

void TransientImageNode::setFiltering(QSGTexture::Filtering filtering)
{
    if (m_material.filtering() == filtering) {
        return;
    }

    m_material.setFiltering(filtering);
    m_opaqueMaterial.setFiltering(filtering);
    markDirty(QSGNode::DirtyMaterial);
}

void TransientImageNode::rebuildGeometry()
{
    QSGTexture *texture = m_material.texture();
    if (texture) {
        QSGGeometry::updateTexturedRectGeometry(&m_geometry, m_targetRect, texture->convertToNormalizedSourceRect(m_sourceRect));
    }
}
