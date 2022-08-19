/*
    SPDX-FileCopyrightText: 2020 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "imagecolors.h"

#include <array>
#include <cmath>
#include <thread>

#include <QGuiApplication>
#include <QPalette>
#include <QQuickItem>

#include "colorutils.h"
#include "wallpapercolors.h"

ImageColors::ImageColors(QObject *parent)
    : QObject(parent)
{
}

void ImageColors::setSourceItem(QQuickItem *source)
{
    if (source == m_sourceItem) {
        return;
    }

    m_sourceItem = source;
    Q_EMIT sourceChanged();

    update();
}

QQuickItem *ImageColors::sourceItem() const
{
    return m_sourceItem;
}

QColor ImageColors::accentColor() const
{
    return m_accentColor;
}

void ImageColors::setBackgroundColor(const QColor &color)
{
    if (color == m_backgroundColor) {
        return;
    }

    m_backgroundColor = color;
    Q_EMIT backgroundColorChanged();

    update();
}

QColor ImageColors::backgroundColor() const
{
    return m_backgroundColor;
}

void ImageColors::update()
{
    if (!m_sourceItem) {
        return;
    }

    if (m_grabResult) {
        disconnect(m_grabResult.data(), nullptr, this, nullptr);
        m_grabResult.clear();
    }

    m_grabResult = m_sourceItem->grabToImage(m_sourceItem->size().scaled(384, 384, Qt::KeepAspectRatio).toSize());
    if (m_grabResult) {
        connect(m_grabResult.data(), &QQuickItemGrabResult::ready, this, &ImageColors::slotGrabResultReady);
    }
}

void ImageColors::slotGrabResultReady()
{
    const QImage &grabImage = m_grabResult.data()->image();
    m_grabResult.clear();

    if (grabImage.isNull()) {
        return;
    }

    std::thread(std::bind(&ImageColors::generateAccentColor, this, grabImage)).detach();
}

void ImageColors::generateAccentColor(const QImage &image)
{
    if (!m_backgroundColor.isValid()) {
        return;
    }

    std::vector<QRgb> samples;
    samples.reserve(image.width() * image.height());
    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            const QColor &sampleColor = image.pixelColor(x, y);
            if (sampleColor.alpha() == 0) {
                continue;
            }

            const QRgb rgb = sampleColor.rgb();
            samples.emplace_back(rgb);
        }
    }

    if (samples.empty()) {
        return;
    }

    generateDominantColor(samples);

    Q_EMIT accentColorChanged();
}

void ImageColors::generateDominantColor(const std::vector<QRgb> &samples)
{
    // 192 is from kcm_colors
    const auto &wallpaperColors = WallpaperColors::fromBitmap(samples);
    m_accentColor = QColor(wallpaperColors.getPrimaryColor());
}
