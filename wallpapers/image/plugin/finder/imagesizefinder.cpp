/*
    SPDX-FileCopyrightText: 2007 Paolo Capriotti <p.capriotti@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imagesizefinder.h"

#include <QImageReader>

ImageSizeFinder::ImageSizeFinder(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path)
{
}

void ImageSizeFinder::run()
{
    const QImageReader reader(m_path);
    Q_EMIT sizeFound(m_path, reader.size());
}
