/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QImage>
#include <QRect>

/**
 * Resets base point to totalRect.topLeft()
 */
inline QPoint desktopStartPoint(const QRect &desktopRect, const QRect &totalRect, int x, int y)
{
    return desktopRect.topLeft() + QPoint(x, y) - totalRect.topLeft();
}

inline QImage cropImage(const QImage &image, const QRect &totalRect, const QRect &desktopRect, const QSize &requestedSize)
{
    QImage cropped;

    // Only reserve the part that should be shown on the specified screen,
    // and better crop to the center area.
    double hWRatio = image.height() / static_cast<double>(image.width());
    double newWidth, newHeight;

    if (totalRect.width() > totalRect.height()) {
        newWidth = totalRect.width();
        newHeight = newWidth * hWRatio;

        if (newHeight < totalRect.height()) {
            newWidth *= totalRect.height() / newHeight;
            newHeight = newWidth * hWRatio;
        }
    } else {
        newHeight = totalRect.height();
        newWidth = newHeight / hWRatio;

        if (newWidth < totalRect.width()) {
            newHeight *= totalRect.width() / newWidth;
            newWidth = newHeight / hWRatio;
        }
    }

    cropped = image.scaled(QSize(newWidth, newHeight), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    const int x = (cropped.width() - totalRect.width()) / 2;
    const int y = (cropped.height() - totalRect.height()) / 2;

    return cropped.copy(QRect(desktopStartPoint(desktopRect, totalRect, x, y), requestedSize));
}
