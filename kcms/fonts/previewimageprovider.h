/*
    SPDX-FileCopyrightText: 2018 Julian Wolff <wolff@julianwolff.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef __PREVIEW_IMAGE_PROVIDER_H__
#define __PREVIEW_IMAGE_PROVIDER_H__

#include <QFont>
#include <QQuickImageProvider>

class PreviewImageProvider : public QQuickImageProvider
{
public:
    PreviewImageProvider(const QFont &font);
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QFont m_font;
};

#endif // __PREVIEW_IMAGE_PROVIDER_H__
