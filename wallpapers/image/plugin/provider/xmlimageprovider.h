/*
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef XMLIMAGEPROVIDER_H
#define XMLIMAGEPROVIDER_H

#include <QQuickAsyncImageProvider>
#include <QThreadPool>

/**
 * A custom image provider for XML wallpapers
 */
class XmlImageProvider : public QQuickAsyncImageProvider
{
public:
    explicit XmlImageProvider();

    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

private:
    QThreadPool m_pool;
};

#endif // XMLIMAGEPROVIDER_H
