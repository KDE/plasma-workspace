/*
    SPDX-FileCopyrightText: 2017 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "wallpapercolors.h"

#include <cmath>

#include "colorutils.h"
#include "palette/celebiquantizer.h"

WallpaperColors::WallpaperColors(const std::map<QRgb, unsigned int> &colorToPopulation, const std::vector<QRgb> &palette, int colorHints)
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
    for (const auto &colorToScoreEntry : std::as_const(mapEntries)) {
        colorsByScoreDescending.emplace_back(colorToScoreEntry.first);
    }

    m_mainColors.reserve(colorsByScoreDescending.size());
    for (const QRgb color : std::as_const(colorsByScoreDescending)) {
        const Cam &cam = colorToCam.at(color);
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
    m_colorHints = colorHints;
}

WallpaperColors WallpaperColors::fromBitmap(const std::vector<QRgb> &pixels)
{
    Q_ASSERT(!pixels.empty());

    CelebiQuantizer quantizer;
    quantizer.quantize(pixels, 128);

    const auto &colorToPopulation = quantizer.inputPixelToCount();
    const auto &palette = quantizer.getQuantizedColors();

    const int hints = calculateDarkHints(pixels);

    return WallpaperColors(colorToPopulation, palette, HINT_FROM_BITMAP | hints);
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

const std::map<QRgb, unsigned int> &WallpaperColors::getAllColors() const
{
    return m_allColors;
}

bool WallpaperColors::operator=(const WallpaperColors &other) const
{
    return m_mainColors == other.m_mainColors && m_allColors == other.m_allColors && m_colorHints == other.m_colorHints;
}

int WallpaperColors::getColorHints() const
{
    return m_colorHints;
}

WallpaperColors::operator std::string() const
{
    std::string colors = "[WallpaperColors: ";
    for ([[maybe_unused]] const QRgb rgb : std::as_const(m_mainColors)) {
        // TODO C++20 enable the code
        // colors.append(std::format("{:x}", rgb));
    }
    colors.append("h: ");
    colors.append(std::to_string(m_colorHints));
    colors.append("]");
    return colors;
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
        proportions[hue] += ((double)population / totalPopulation);
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

int WallpaperColors::calculateDarkHints(const std::vector<QRgb> pixels)
{
    if (pixels.empty()) {
        return 0;
    }

    double totalLuminance = 0;
    const int maxDarkPixels = pixels.size() * MAX_DARK_AREA;
    int darkPixels = 0;

    // This bitmap was already resized to fit the maximum allowed area.
    // Let's just loop through the pixels, no sweat!
    std::array<double, 3> tmpHsl;
    tmpHsl.fill(0.0);
    for (std::size_t i = 0; i < pixels.size(); i++) {
        const QRgb pixelColor = pixels[i];
        ColorUtils::colorToHSL(pixelColor, tmpHsl);

        // Calculate the luminance of the wallpaper pixel color.
        double adjustedLuminance = ColorUtils::calculateLuminance(pixelColor);

        // Make sure we don't have a dark pixel mass that will
        // make text illegible.
        bool satisfiesTextContrast = ColorUtils::calculateContrast(pixelColor, 0xFF000000) > DARK_PIXEL_CONTRAST;
        if (!satisfiesTextContrast) {
            darkPixels++;
        }
        totalLuminance += adjustedLuminance;
    }

    int hints = 0;
    double meanLuminance = totalLuminance / (double)pixels.size();
    if (meanLuminance > BRIGHT_IMAGE_MEAN_LUMINANCE && darkPixels < maxDarkPixels) {
        hints |= HINT_SUPPORTS_DARK_TEXT;
    }
    if (meanLuminance < DARK_THEME_MEAN_LUMINANCE) {
        hints |= HINT_SUPPORTS_DARK_THEME;
    }

    return hints;
}
