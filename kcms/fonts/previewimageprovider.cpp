/*
    SPDX-FileCopyrightText: 2018 Julian Wolff <wolff@julianwolff.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <cmath>

#include <QApplication>
#include <QPainter>
#include <QPalette>

#include <KWindowSystem>

#include "kxftconfig.h"
#include "previewimageprovider.h"
#include "previewrenderengine.h"

/**
 * @brief Combines a list of images vertically into one image
 *
 * Combines a list of images vertically with the given spacing
 * between adjacent rows of images.
 * The width of the combined image is the width of the longest
 * image in the list, and the height is the sum of the heights
 * of all images in the list.
 * The device pixel ratio of the combined image is the same
 * as that of the first image in the list.
 *
 * @param images the list of images to be combined
 * @param bgnd the background color of the combined image
 * @param _spacing the spacing between adjacent rows of images
 * @return the combined image
 */
QImage combineImages(const QList<QImage> &images, const QColor &bgnd, int _spacing = 0)
{
    if (images.empty()) {
        return QImage();
    }

    int width = 0;
    int height = 0;
    const double devicePixelRatio = images.at(0).devicePixelRatio();
    const int spacing = std::lround(_spacing * devicePixelRatio);
    for (const auto &image : images) {
        if (width < image.width()) {
            width = image.width();
        }
        height += image.height();
    }
    height += spacing * images.count();

    // To correctly align the image pixels on a high dpi display,
    // the image dimensions need to be a multiple of devicePixelRatio
    QImage combinedImage(width * devicePixelRatio, height * devicePixelRatio, images.at(0).format());
    combinedImage.setDevicePixelRatio(devicePixelRatio);
    combinedImage.fill(bgnd);

    int offset = spacing; // Top margin
    QPainter p(&combinedImage);
    for (const auto &image : images) {
        p.drawImage(0, offset, image);
        offset += image.height() + spacing;
    }

    return combinedImage;
}

PreviewImageProvider::PreviewImageProvider(const QFont &font)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_font(font)
{
}

QImage PreviewImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)
    if (!KWindowSystem::isPlatformX11()) {
        return QImage();
    }

    int subPixelIndex = 0;
    int hintingIndex = 0;

    const auto idpart = QStringView(id).split(QLatin1Char('.'))[0];
    const auto sections = idpart.split(QLatin1Char('_'));

    if (sections.size() >= 2) {
        subPixelIndex = sections[0].toInt() + KXftConfig::SubPixel::None;
        hintingIndex = sections[1].toInt() + KXftConfig::Hint::None;
    } else {
        return QImage();
    }

    KXftConfig xft;

    KXftConfig::AntiAliasing::State oldAntialiasing = xft.getAntiAliasing();
    double oldStart = 0;
    double oldEnd = 0;
    xft.getExcludeRange(oldStart, oldEnd);
    KXftConfig::SubPixel::Type oldSubPixelType = KXftConfig::SubPixel::NotSet;
    xft.getSubPixelType(oldSubPixelType);
    KXftConfig::Hint::Style oldHintStyle = KXftConfig::Hint::NotSet;
    xft.getHintStyle(oldHintStyle);

    xft.setAntiAliasing(KXftConfig::AntiAliasing::Enabled);
    xft.setExcludeRange(0, 0);

    KXftConfig::SubPixel::Type subPixelType = (KXftConfig::SubPixel::Type)subPixelIndex;
    xft.setSubPixelType(subPixelType);

    KXftConfig::Hint::Style hintStyle = (KXftConfig::Hint::Style)hintingIndex;
    xft.setHintStyle(hintStyle);

    xft.apply();

    QColor text(QApplication::palette().color(QPalette::Text));
    QColor bgnd(QApplication::palette().color(QPalette::Window));

    PreviewRenderEngine eng(true);
    QList<QImage> lines;

    lines << eng.drawAutoSize(m_font, text, bgnd, eng.getDefaultPreviewString());

    QImage img = combineImages(lines, bgnd, lines[0].height() * .25);

    xft.setAntiAliasing(oldAntialiasing);
    xft.setExcludeRange(oldStart, oldEnd);
    xft.setSubPixelType(oldSubPixelType);
    xft.setHintStyle(oldHintStyle);

    xft.apply();

    *size = img.size();

    return img;
}
