/*
    SPDX-FileCopyrightText: 2018 Julian Wolff <wolff@julianwolff.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QApplication>
#include <QPalette>

#include "kxftconfig.h"
#include "previewimageprovider.h"
#include "previewrenderengine.h"

PreviewImageProvider::PreviewImageProvider(const QFont &font)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_font(font)
{
}

QImage PreviewImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(requestedSize)

    int subPixelIndex;
    int hintingIndex;
    qreal dpr;

    const auto sections = id.split(QLatin1Char('_'));
    if (sections.size() >= 3) {
        subPixelIndex = sections[0].toInt() + KXftConfig::SubPixel::None;
        hintingIndex = sections[1].toInt() + KXftConfig::Hint::None;
        dpr = sections[2].toDouble();
    } else {
        return {};
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

    auto subPixelType = (KXftConfig::SubPixel::Type)subPixelIndex;
    xft.setSubPixelType(subPixelType);

    auto hintStyle = (KXftConfig::Hint::Style)hintingIndex;
    xft.setHintStyle(hintStyle);

    xft.apply();

    const QColor textColor(QApplication::palette().color(QPalette::Text));
    const QColor backgroundColor(QApplication::palette().color(QPalette::Window));

    PreviewRenderEngine eng(true);
    QImage img = eng.drawAutoSize(m_font, dpr, textColor, backgroundColor, eng.getDefaultPreviewString());

    xft.setAntiAliasing(oldAntialiasing);
    xft.setExcludeRange(oldStart, oldEnd);
    xft.setSubPixelType(oldSubPixelType);
    xft.setHintStyle(oldHintStyle);

    xft.apply();

    *size = img.size();

    return img;
}
