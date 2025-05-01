/*
    SPDX-FileCopyrightText: 2022 Janet Blackquill <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QQuickItem>

class MaskMouseArea : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

    struct Private;
    QScopedPointer<Private> d;

    void updateMask();

protected:
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void hoverEnterEvent(QHoverEvent *event) override;
    void hoverLeaveEvent(QHoverEvent *event) override;

public:
    explicit MaskMouseArea(QQuickItem *parent = nullptr);
    ~MaskMouseArea();

    bool contains(const QPointF &point) const override;

    Q_PROPERTY(bool hovered READ hovered NOTIFY hoveredChanged)
    bool hovered() const;
    Q_SIGNAL void hoveredChanged();

    Q_SIGNAL void tapped();
};
