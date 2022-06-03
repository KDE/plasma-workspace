/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef WIDEIMAGEPROVIDER_H
#define WIDEIMAGEPROVIDER_H

#include <QQuickAsyncImageProvider>
#include <QThreadPool>

/**
 * Wide image (spanning multiple screens) provider
 * URL passed to this provider:
 * image://wideimage/get?path=<Image Path>
 *  &desktopX=<Desktop X>
 *  &desktopY=<Desktop Y>
 *  &desktopWidth=<Desktop Width>
 *  &desktopHeight=<Desktop Height>
 *  &totalRectX=<Total X>&totalRectY=<Total X>
 *  &totalRectWidth=<Total Width>
 *  &totalRectHeight=<Total Height>
 */
class WideImageProvider : public QQuickAsyncImageProvider
{
public:
    explicit WideImageProvider();

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

private:
    QThreadPool m_pool;
};

#endif // WIDEIMAGEPROVIDER_H
