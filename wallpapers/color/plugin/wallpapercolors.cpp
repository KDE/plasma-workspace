/*
    SPDX-FileCopyrightText: 2017 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "wallpapercolors.h"

#include <cmath>
#include <numeric>

#include "palette/celebiquantizer.h"

constexpr QRgb PLASMA_BLUE = ((611 & 0x0ff) << 16) | ((174 & 0x0ff) << 8) | (233 & 0x0ff);

WallpaperColors::WallpaperColors(const std::map<QRgb, unsigned int> &colorToPopulation, const std::vector<QRgb> &palette)
{
    m_allColors = colorToPopulation;

    std::map<QRgb, Cam> colorToCam;
    for (const QRgb rgb : palette) {
        colorToCam.emplace(rgb, Cam::fromInt(rgb));
    }
    const auto &hueProportions = getHueProportions(colorToCam, colorToPopulation);
    const auto &colorToHueProportion = getColorToHueProportion(palette, colorToCam, hueProportions);

    std::map<QRgb, double> colorToScore;
    for (const auto &[color, proportion] : colorToHueProportion) {
        double score = getScore(colorToCam[color], proportion);
        colorToScore.emplace(color, score);
    }
    std::vector<std::pair<QRgb, double>> mapEntries(colorToScore.cbegin(), colorToScore.cend());
    std::sort(mapEntries.begin(), mapEntries.end(), [](const std::pair<QRgb, double> &a, decltype(a) b) {
        return b.second < a.second;
    });

    std::vector<QRgb> colorsByScoreDescending;
    colorsByScoreDescending.reserve(mapEntries.size());
    std::transform(mapEntries.cbegin(), mapEntries.cend(), std::back_inserter(colorsByScoreDescending), [](const std::pair<QRgb, double> &colorToScoreEntry) {
        return colorToScoreEntry.first;
    });

    m_mainColors.reserve(colorsByScoreDescending.size());
    for (const QRgb color : std::as_const(colorsByScoreDescending)) {
        const Cam &cam = colorToCam.at(color);
        if (cam.getChroma() < 5) {
            // CHANGED: Additional filter conditions
            continue;
        }
        bool skip = false;
        for (const QRgb otherColor : std::as_const(m_mainColors)) {
            const Cam &otherCam = colorToCam.at(otherColor);
            if (cam - otherCam < 15) {
                skip = true;
                break;
            }
        }
        if (!skip) {
            m_mainColors.emplace_back(color);
        }
    }

    if (m_mainColors.empty()) {
        m_mainColors.emplace_back(PLASMA_BLUE);
    }
}

WallpaperColors WallpaperColors::fromBitmap(const std::vector<QRgb> &pixels)
{
    Q_ASSERT(!pixels.empty());

    CelebiQuantizer quantizer;
    quantizer.quantize(pixels, 128);

    const auto &colorToPopulation = quantizer.inputPixelToCount();
    const auto &palette = quantizer.getQuantizedColors();

    return WallpaperColors(colorToPopulation, palette);
}

QRgb WallpaperColors::getPrimaryColor() const
{
    return m_mainColors.at(0);
}

std::optional<QRgb> WallpaperColors::getSecondaryColor() const
{
    if (m_mainColors.size() < 2) {
        return std::nullopt;
    }

    return m_mainColors.at(1);
}

std::optional<QRgb> WallpaperColors::getTertiaryColor() const
{
    if (m_mainColors.size() < 3) {
        return std::nullopt;
    }

    return m_mainColors.at(2);
}

const std::vector<QRgb> &WallpaperColors::getMainColors() const
{
    return m_mainColors;
}

bool WallpaperColors::operator=(const WallpaperColors &other) const
{
    return m_mainColors == other.m_mainColors && m_allColors == other.m_allColors;
}

std::array<double, 360> WallpaperColors::getHueProportions(const std::map<QRgb, Cam> &colorToCam, const std::map<QRgb, unsigned> &colorToPopulation)
{
    std::array<double, 360> proportions;
    proportions.fill(0.0);

    double totalPopulation = 0.0;
    for (const auto &entry : colorToPopulation) {
        totalPopulation += entry.second;
    }

    for (const auto &entry : colorToPopulation) {
        const QRgb color = entry.first;
        const unsigned population = colorToPopulation.at(color);
        const Cam &cam = colorToCam.at(color);
        const int hue = wrapDegrees(std::lround(cam.getHue()));
        proportions[hue] += population / totalPopulation;
    }

    return proportions;
}

std::map<QRgb, double>
WallpaperColors::getColorToHueProportion(const std::vector<QRgb> &colors, std::map<QRgb, Cam> &colorToCam, const std::array<double, 360> &hueProportions)
{
    std::map<QRgb, double> colorToHueMap;
    for (const QRgb color : colors) {
        const int hue = wrapDegrees(std::lround(colorToCam.at(color).getHue()));
        double proportion = 0.0;
        for (int i = hue - 15; i < hue + 15; i++) {
            proportion += hueProportions[wrapDegrees(i)];
        }
        colorToHueMap.emplace(color, proportion);
    }
    return colorToHueMap;
}

double WallpaperColors::getScore(const Cam &cam, double proportion)
{
    return cam.getChroma() + (proportion * 100);
}

int WallpaperColors::wrapDegrees(int degrees)
{
    if (degrees < 0) {
        return (degrees % 360) + 360;
    } else if (degrees >= 360) {
        return degrees % 360;
    } else {
        return degrees;
    }
}
