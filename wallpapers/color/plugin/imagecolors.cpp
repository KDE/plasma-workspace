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

#define WCAG_NON_TEXT_CONTRAST_RATIO 3
#define WCAG_TEXT_CONTRAST_RATIO 4.5
#define MINIMUM_REQUIRED_SAMPLE_PERCENT 0.01

#define SATURATION_PRIORITIZE_COEFF 1.0
#define MINIMUM_SATURATIONF 0.2
#define DARK_BACKGROUND_UPPER_LUM 0.15
#define LIGHT_BACKGROUND_LOWER_LUM 0.7
#define DARK_UPPER_LUM 0.95

namespace
{
inline int squareDistance(QRgb color1, QRgb color2)
{
    // https://en.wikipedia.org/wiki/Color_difference
    // Using RGB distance for performance, as CIEDE2000 istoo complicated
    if (qRed(color1) - qRed(color2) < 128) {
        return 2 * pow(qRed(color1) - qRed(color2), 2) //
            + 4 * pow(qGreen(color1) - qGreen(color2), 2) //
            + 3 * pow(qBlue(color1) - qBlue(color2), 2);
    } else {
        return 3 * pow(qRed(color1) - qRed(color2), 2) //
            + 4 * pow(qGreen(color1) - qGreen(color2), 2) //
            + 2 * pow(qBlue(color1) - qBlue(color2), 2);
    }
}

}

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

    // Find acceptable luminance range
    const qreal backgroundLum = luminance(m_backgroundColor);
    qreal lowerLum = 0, upperLum = 1;
    if (backgroundLum <= DARK_BACKGROUND_UPPER_LUM) {
        // (lowerLum + 0.05) / (backgroundLum + 0.05) >= 3
        lowerLum = WCAG_NON_TEXT_CONTRAST_RATIO * (backgroundLum + 0.05) - 0.05;
        upperLum = DARK_UPPER_LUM;
    } else if (backgroundLum >= LIGHT_BACKGROUND_LOWER_LUM) {
        // For light themes, still prefer lighter colors
        // Assuming the luminance of text is 0, so (lowerLum + 0.05) / (0 + 0.05) >= 4.5
        lowerLum = WCAG_TEXT_CONTRAST_RATIO * 0.05 - 0.05;
        upperLum = backgroundLum;
    }

    std::vector<QRgb> samples;
    samples.reserve(image.width() * image.height());
    unsigned count = 0;

    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            const QColor &sampleColor = image.pixelColor(x, y);
            if (sampleColor.alpha() == 0) {
                continue;
            }

            count++;

            // Saturation filter
            if (sampleColor.saturationF() < MINIMUM_SATURATIONF) {
                continue;
            }

            const QRgb rgb = sampleColor.rgb();
            samples.emplace_back(rgb);
        }
    }

    if (count == 0) {
        return;
    }

    std::vector<ColorStat> clusters;
    const std::size_t lowerCount = count * MINIMUM_REQUIRED_SAMPLE_PERCENT;
    if (samples.size() >= lowerCount) {
        // Vibrant samples available, but need care WCAG
        for (const QRgb rgb : std::as_const(samples)) {
            positionColor(rgb, clusters);
        }
        generateDominantColor(clusters, samples);

        // Adjust saturation to make the color more vibrant
        if (m_accentColor.hsvSaturationF() < 0.5) {
            const double h = m_accentColor.hsvHueF();
            const double v = m_accentColor.valueF();
            m_accentColor.setHsvF(h, 0.5, v);
        }

        unsigned colorOperationCount = 0;
        while (luminance(m_accentColor) < lowerLum && colorOperationCount++ < 10) {
            m_accentColor = m_accentColor.lighter(110);
        }
        while (luminance(m_accentColor) > upperLum && colorOperationCount++ < 10) {
            m_accentColor = m_accentColor.darker(110);
        }
    } else {
        // Fall back to Breeze Blue
        m_accentColor = ((611 & 0x0ff) << 16) | ((174 & 0x0ff) << 8) | (233 & 0x0ff);
    }

    Q_EMIT accentColorChanged();
}

void ImageColors::generateDominantColor(std::vector<ColorStat> &clusters, std::vector<QRgb> &samples)
{
    unsigned r2 = 0, g2 = 0, b2 = 0, c2 = 0;

    for (int iteration = 0; iteration < 5; ++iteration) {
        for (auto &stat : clusters) {
            r2 = 0;
            g2 = 0;
            b2 = 0;
            c2 = 0;

            for (auto color : std::as_const(stat.colors)) {
                c2++;
                r2 += qRed(color);
                g2 += qGreen(color);
                b2 += qBlue(color);
            }
            r2 = r2 / c2;
            g2 = g2 / c2;
            b2 = b2 / c2;
            stat.centroid = qRgb(r2, g2, b2);
            stat.colors = {stat.centroid};
        }

        for (auto color : std::as_const(samples)) {
            positionColor(color, clusters);
        }
    }

    std::sort(clusters.begin(), clusters.end(), [](const ColorStat &a, const ColorStat &b) {
        return a.colors.size() * std::exp(-SATURATION_PRIORITIZE_COEFF / (QColor(a.centroid).saturationF() + 1e-7))
            > b.colors.size() * std::exp(-SATURATION_PRIORITIZE_COEFF / (QColor(b.centroid).saturationF() + 1e-7));
    });

    m_accentColor = QColor(clusters.cbegin()->centroid);
}

void ImageColors::positionColor(QRgb rgb, std::vector<ColorStat> &clusters)
{
    for (auto &stat : clusters) {
        if (squareDistance(rgb, stat.centroid) < s_minimumSquareDistance) {
            stat.colors.push_back(rgb);
            return;
        }
    }

    ColorStat stat;
    stat.colors.emplace_back(rgb);
    stat.centroid = rgb;
    clusters.emplace_back(stat);
}

qreal ImageColors::luminance(const QColor &color) const
{
    std::array<qreal, 3> rgb{color.redF(), color.greenF(), color.blueF()}; // range 0-1
    std::transform(rgb.cbegin(), rgb.cend(), rgb.begin(), [](qreal v) {
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    });

    return rgb[0] * 0.2126 + rgb[1] * 0.7152 + rgb[2] * 0.0722;
}
