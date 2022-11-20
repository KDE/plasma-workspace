/*
    SPDX-FileCopyrightText: 2022 Janet Blackquill <uhhadd@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "maskmousearea.h"
#include <QQuickItemGrabResult>

struct MaskMouseArea::Private {
    QImage maskImage;
    bool containsMouse = false;
};

MaskMouseArea::MaskMouseArea(QQuickItem *parent)
    : QQuickItem(parent)
    , d(new Private)
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    updateMask();
    connect(this, &QQuickItem::parentChanged, this, &MaskMouseArea::updateMask);
    connect(this, &QQuickItem::parentChanged, this, &MaskMouseArea::updateMask);
}

MaskMouseArea::~MaskMouseArea()
{
}

void MaskMouseArea::updateMask()
{
    if (parentItem() == nullptr) {
        return;
    }

    auto result = parentItem()->grabToImage();
    connect(result.data(), &QQuickItemGrabResult::ready, this, [=] {
        d->maskImage = result->image();
    });
}

void MaskMouseArea::geometryChangedFn(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry)
    Q_UNUSED(oldGeometry)

    updateMask();
}

bool MaskMouseArea::contains(const QPointF &point) const
{
    if (!QQuickItem::contains(point) || d->maskImage.isNull()) {
        return false;
    }

    QPoint rounded = point.toPoint();
    const int x = rounded.x(), y = rounded.y();

    if (x < 0 || x >= d->maskImage.width()) {
        return false;
    }
    if (y < 0 || y >= d->maskImage.height()) {
        return false;
    }

    return d->maskImage.pixel(x, y) != 0;
}

void MaskMouseArea::mousePressEvent(QMouseEvent *event)
{
    event->accept();
}

void MaskMouseArea::mouseReleaseEvent(QMouseEvent *event)
{
    if (!contains(event->localPos())) {
        return;
    }

    Q_EMIT tapped();
}

void MaskMouseArea::hoverEnterEvent(QHoverEvent *event)
{
    Q_UNUSED(event)
    d->containsMouse = true;
    Q_EMIT hoveredChanged();
}

void MaskMouseArea::hoverLeaveEvent(QHoverEvent *event)
{
    Q_UNUSED(event)
    d->containsMouse = false;
    Q_EMIT hoveredChanged();
}

bool MaskMouseArea::hovered() const
{
    return d->containsMouse;
}
