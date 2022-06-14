/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "wideimage.h"

#include <QQuickWindow>
#include <QRectF>
#include <QScreen>
#include <QSizeF>

#include <KWindowSystem>

QRectF WideImage::cropRect(QQuickWindow *targetWindow, const QRectF &_boundingRect)
{
    if (!targetWindow || !_boundingRect.isValid()) {
        return {};
    }

    const QSizeF itemSize = targetWindow->size();

    // Only reserve the part that should be shown on the specified screen,
    // and better crop to the center area.
    const double itemWHRatio = itemSize.width() / itemSize.height();
    const double boundingWHRatio = _boundingRect.width() / _boundingRect.height();

    // Scale boundingRect to fit the original size
    QRectF boundingRect;
    if (itemWHRatio < boundingWHRatio) {
        // For wide screens [Screen 1] [Screen 2]
        boundingRect = QRectF(_boundingRect.topLeft() * itemSize.width() / _boundingRect.width(), QSizeF(itemSize.width(), itemSize.width() / boundingWHRatio));
    } else {
        // For tall screens
        // [Screen 1]
        // [Screen 2]
        boundingRect =
            QRectF(_boundingRect.topLeft() * itemSize.height() / _boundingRect.height(), QSizeF(itemSize.height() * boundingWHRatio, itemSize.height()));
    }

    // Considers device pixel ratio and calculates the real desktop geometry.
    const double devicePixelRatio = KWindowSystem::isPlatformX11() ? targetWindow->screen()->devicePixelRatio() : 1;
    const QRectF desktopRect(targetWindow->x(), targetWindow->y(), targetWindow->width() * devicePixelRatio, targetWindow->height() * devicePixelRatio);

    // Scale desktopRect to match the scaled boundingRect
    const double scaleRatio = boundingRect.width() / static_cast<double>(_boundingRect.width());
    const QRectF targetRect(desktopRect.topLeft() * scaleRatio, desktopRect.size() * scaleRatio);

    const double offsetX = (itemSize.width() - boundingRect.width()) / 2; // For tall wallpapers
    const double offsetY = (itemSize.height() - boundingRect.height()) / 2; // For long wallpapers

    // Resets base point to boundingRect.topLeft()
    return targetRect.translated(QPointF(offsetX, offsetY) - boundingRect.topLeft());
}
