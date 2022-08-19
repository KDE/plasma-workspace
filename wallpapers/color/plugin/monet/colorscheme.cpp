/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "colorscheme.h"

#include <numeric>
#include <unordered_set>

#include <cmath>

#include "shades.h"
#include "wallpapercolors.h"

#define ACCENT1_CHROMA 48.0
#define MIN_CHROMA 5
constexpr QRgb PLASMA_BLUE = ((61 & 0x0ff) << 16) | ((174 & 0x0ff) << 8) | (233 & 0x0ff);

double Hue::getHueRotation(double sourceHue, const std::array<std::pair<int, int>, 9> &hueAndRotations) const
{
    const double sanitizedSourceHue = (sourceHue < 0 || sourceHue >= 360) ? 0 : sourceHue;
    for (std::size_t i = 1; i <= hueAndRotations.size() - 2; ++i) {
        const double thisHue = hueAndRotations[i].first;
        const double nextHue = hueAndRotations[i + 1].first;
        if (thisHue <= sanitizedSourceHue && sanitizedSourceHue < nextHue) {
            return ColorScheme::wrapDegreesDouble(sanitizedSourceHue + hueAndRotations[i].second);
        }
    }

    // If this statement executes, something is wrong, there should have been a rotation
    // found using the arrays.
    return sourceHue;
}

double HueSource::get(const Cam &sourceColor) const
{
    return sourceColor.getHue();
}

HueAdd::HueAdd(double amountDegrees)
    : Hue()
    , m_amountDegrees(amountDegrees)
{
}

double HueAdd::get(const Cam &sourceColor) const
{
    return ColorScheme::wrapDegreesDouble(sourceColor.getHue() + m_amountDegrees);
}

HueSubtract::HueSubtract(double amountDegrees)
    : Hue()
    , m_amountDegrees(amountDegrees)
{
}

double HueSubtract::get(const Cam &sourceColor) const
{
    return ColorScheme::wrapDegreesDouble(sourceColor.getHue() - m_amountDegrees);
}

double HueVibrantSecondary::get(const Cam &sourceColor) const
{
    return getHueRotation(sourceColor.getHue(), m_hueToRotations);
}

double HueVibrantTertiary::get(const Cam &sourceColor) const
{
    return getHueRotation(sourceColor.getHue(), m_hueToRotations);
}

double HueExpressiveSecondary::get(const Cam &sourceColor) const
{
    return getHueRotation(sourceColor.getHue(), m_hueToRotations);
}

double HueExpressiveTertiary::get(const Cam &sourceColor) const
{
    return getHueRotation(sourceColor.getHue(), m_hueToRotations);
}

double ChromaMaxOut::get(const Cam &) const
{
    // Intentionally high. Gamut mapping from impossible HCT to sRGB will ensure that
    // the maximum chroma is reached, even if lower than this constant.
    return 130.0;
}

ChromaMultiple::ChromaMultiple(double multiple)
    : m_multiple(multiple)
{
}

double ChromaMultiple::get(const Cam &sourceColor) const
{
    return sourceColor.getChroma() * m_multiple;
}

ChromaConstant::ChromaConstant(double chroma)
    : m_chroma(chroma)
{
}

double ChromaConstant::get(const Cam &) const
{
    return m_chroma;
}

ChromaSource::ChromaSource()
    : ChromaMultiple(1.0)
{
}

TonalSpec::TonalSpec(Hue *hue, Chroma *chroma)
    : m_hue(hue)
    , m_chroma(chroma)
{
}

std::array<QRgb, 12> TonalSpec::shades(const Cam &sourceColor) const
{
    const double hue = m_hue->get(sourceColor);
    const double chroma = m_chroma->get(sourceColor);
    return Shades::of(hue, chroma);
}

CoreSpec style(Style s)
{
    switch (s) {
    case Style::SPRITZ:
        return CoreSpec{TonalSpec(new HueSource(), new ChromaConstant(12.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(8.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(16.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(2.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(2.0))};
    case Style::VIBRANT:
        return CoreSpec{TonalSpec(new HueSource(), new ChromaMaxOut()),
                        TonalSpec(new HueVibrantSecondary(), new ChromaConstant(24.0)),
                        TonalSpec(new HueVibrantTertiary(), new ChromaConstant(32.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(10.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(12.0))};
    case Style::EXPRESSIVE:
        return CoreSpec{TonalSpec(new HueAdd(240.0), new ChromaConstant(40.0)),
                        TonalSpec(new HueExpressiveSecondary(), new ChromaConstant(24.0)),
                        TonalSpec(new HueExpressiveTertiary(), new ChromaConstant(32.0)),
                        TonalSpec(new HueAdd(15.0), new ChromaConstant(8.0)),
                        TonalSpec(new HueAdd(15.0), new ChromaConstant(12.0))};
    case Style::RAINBOW:
        return CoreSpec{TonalSpec(new HueSource(), new ChromaConstant(48.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(16.0)),
                        TonalSpec(new HueAdd(60.0), new ChromaConstant(24.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(0.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(0.0))};
    case Style::FRUIT_SALAD:
        return CoreSpec{TonalSpec(new HueSubtract(50.0), new ChromaConstant(48.0)),
                        TonalSpec(new HueSubtract(50.0), new ChromaConstant(36.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(36.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(10.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(16.0))};
    case Style::CONTENT:
        return CoreSpec{TonalSpec(new HueSource(), new ChromaSource()),
                        TonalSpec(new HueSource(), new ChromaMultiple(0.33)),
                        TonalSpec(new HueSource(), new ChromaMultiple(0.66)),
                        TonalSpec(new HueSource(), new ChromaMultiple(0.0833)),
                        TonalSpec(new HueSource(), new ChromaMultiple(0.1666))};
    case Style::VANILLA:
        return CoreSpec{TonalSpec(new HueSource(), new ChromaSource()),
                        TonalSpec(new HueSource(), new ChromaSource()),
                        TonalSpec(new HueSource(), new ChromaSource()),
                        TonalSpec(new HueSource(), new ChromaSource()),
                        TonalSpec(new HueSource(), new ChromaSource())};
    case Style::TONAL_SPOT:
    default:
        return CoreSpec{TonalSpec(new HueSource(), new ChromaConstant(36.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(16.0)),
                        TonalSpec(new HueAdd(60.0), new ChromaConstant(24.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(4.0)),
                        TonalSpec(new HueSource(), new ChromaConstant(8.0))};
    }
}

ColorScheme::ColorScheme(QRgb seed, bool darkTheme, Style style)
    : m_seed(seed)
    , m_darkTheme(darkTheme)
    , m_style(style)
{
    init();
}

ColorScheme::ColorScheme(const WallpaperColors &wallpaperColors, bool darkTheme, Style style)
    : m_seed(getSeedColor(wallpaperColors, style != Style::CONTENT && style != Style::VANILLA)) // NOTE changed
    , m_darkTheme(darkTheme)
    , m_style(style)
{
    init();
}

std::vector<QRgb> ColorScheme::allAccentColors() const
{
    std::vector<QRgb> list;
    list.reserve(m_accent1.size() + m_accent2.size() + m_accent3.size());
    std::copy(m_accent1.cbegin(), m_accent1.cend(), std::back_inserter(list));
    std::copy(m_accent2.cbegin(), m_accent2.cend(), std::back_inserter(list));
    std::copy(m_accent3.cbegin(), m_accent3.cend(), std::back_inserter(list));
    return list;
}

std::vector<QRgb> ColorScheme::allNeutralColors() const
{
    std::vector<QRgb> list;
    list.reserve(m_neutral1.size() + m_neutral2.size());
    std::copy(m_neutral1.cbegin(), m_neutral1.cend(), std::back_inserter(list));
    std::copy(m_neutral2.cbegin(), m_neutral2.cend(), std::back_inserter(list));
    return list;
}

QRgb ColorScheme::backgroundColor() const
{
    return m_darkTheme ? m_neutral1[8] : m_neutral1[0];
}

QRgb ColorScheme::accentColor() const
{
    // NOTE chaged from 6 to 5 to make the color brighter
    return m_darkTheme ? m_accent1[2] : m_accent1[5];
}

ColorScheme::operator std::string() const
{
    std::string temp("ColorScheme {\n");
    temp += "  seed color: " + stringForColor(m_seed) + "\n";
    temp += "  style: $style\n";
    temp += "  palettes: \n";
    temp += "  " + humanReadable("PRIMARY", m_accent1) + "\n";
    temp += "  " + humanReadable("SECONDARY", m_accent2) + "\n";
    temp += "  " + humanReadable("TERTIARY", m_accent3) + "\n";
    temp += "  " + humanReadable("NEUTRAL", m_neutral1) + "\n";
    temp += "  " + humanReadable("NEUTRAL VARIANT", m_neutral2) + "\n";
    temp += "}";
    return temp;
}

QRgb ColorScheme::getSeedColor(const WallpaperColors &wallpaperColors, bool filter) const
{
    return getSeedColors(wallpaperColors, filter).at(0);
}

std::vector<QRgb> ColorScheme::getSeedColors(const WallpaperColors &wallpaperColors, bool filter) const
{
    if (!filter) {
        return wallpaperColors.getMainColors();
    }

    const auto &allColorsMap = wallpaperColors.getAllColors();
    double totalPopulation = 0.0;
    for (const auto &pr : allColorsMap) {
        totalPopulation += pr.second;
    }
    const bool totalPopulationMeaningless = (totalPopulation == 0.0);
    if (totalPopulationMeaningless) {
        // WallpaperColors with a population of 0 indicate the colors didn't come from
        // quantization. Instead of scoring, trust the ordering of the provided primary
        // secondary/tertiary colors.
        //
        // In this case, the colors are usually from a Live Wallpaper.
        const auto &mainColors = wallpaperColors.getMainColors();
        std::unordered_set<QRgb> distinctColors(mainColors.cbegin(), mainColors.cend());
        if (filter) {
            std::vector<QRgb> removedColors;
            for (const QRgb rgb : std::as_const(distinctColors)) {
                if (Cam::fromInt(rgb).getChroma() < MIN_CHROMA) {
                    removedColors.emplace_back(rgb);
                }
            }
            for (const QRgb rgb : std::as_const(removedColors)) {
                distinctColors.erase(rgb);
            }
        }
        if (distinctColors.empty()) {
            return {PLASMA_BLUE};
        }
        return std::vector<QRgb>(distinctColors.cbegin(), distinctColors.cend());
    }

    std::map<QRgb, double> intToProportion;
    for (const auto &pr : allColorsMap) {
        intToProportion.emplace(pr.first, pr.second / totalPopulation);
    }
    std::map<QRgb, Cam> intToCam;
    for (const auto &pr : allColorsMap) {
        intToCam.emplace(pr.first, Cam::fromInt(pr.second));
    }

    // Get an array with 360 slots. A slot contains the percentage of colors with that hue.
    const std::array<double, 360> &hueProportions = getHuePopulations(intToCam, intToProportion, filter);
    // Map each color to the percentage of the image with its hue.
    std::map<QRgb, double> intToHueProportion;
    for (const auto &pr : allColorsMap) {
        const auto &cam = intToCam[pr.first];
        const int hue = std::lround(cam.getHue());
        double proportion = 0.0;
        for (int i = hue - 15; i <= hue + 15; ++i) {
            proportion += hueProportions[wrapDegrees(i)];
        }
        intToHueProportion.emplace(pr.first, proportion);
    }

    // Remove any inappropriate seed colors. For example, low chroma colors look grayscale
    // raising their chroma will turn them to a much louder color that may not have been
    // in the image.
    auto filteredIntToCam = intToCam;
    if (filter) {
        std::vector<QRgb> removedColors;
        for (const auto &pr : std::as_const(filteredIntToCam)) {
            if (pr.second.getChroma() < MIN_CHROMA || intToHueProportion[pr.first] <= 0.005) { // NOTE changed here
                removedColors.emplace_back(pr.first);
            }
        }
        for (const QRgb rgb : std::as_const(removedColors)) {
            filteredIntToCam.erase(rgb);
        }
    }
    // Sort the colors by score, from high to low.
    std::vector<std::pair<QRgb, double>> intToScore;
    intToScore.reserve(filteredIntToCam.size());
    for (const auto &pr : std::as_const(filteredIntToCam)) {
        intToScore.emplace_back(std::make_pair(pr.first, score(pr.second, intToHueProportion[pr.first])));
    }
    std::sort(intToScore.begin(), intToScore.end(), [](const std::pair<QRgb, double> &a, decltype(a) b) {
        return a.second > b.second;
    });

    // Go through the colors, from high score to low score.
    // If the color is distinct in hue from colors picked so far, pick the color.
    // Iteratively decrease the amount of hue distinctness required, thus ensuring we
    // maximize difference between colors.
    constexpr int minimumHueDistance = 15;
    std::vector<QRgb> seeds;
    for (int i = 90; i >= minimumHueDistance; --i) {
        seeds.clear();
        bool finished = false;
        for (const auto &pr : std::as_const(intToScore)) {
            const bool existingSeedNearby = std::any_of(seeds.cbegin(), seeds.cend(), [this, i, &intToCam, &pr](QRgb rgb) {
                const double hueA = intToCam[pr.first].getHue();
                const double hueB = intToCam[rgb].getHue();
                return hueDiff(hueA, hueB) < i;
            });
            if (existingSeedNearby) {
                continue;
            }
            seeds.emplace_back(pr.first);
            if (seeds.size() >= 4) {
                finished = true;
                break;
            }
        }
        if (finished) {
            break;
        }
    }

    if (seeds.empty()) {
        // Use gBlue 500 if there are 0 colors
        seeds.emplace_back(PLASMA_BLUE);
    }

    return seeds;
}

int ColorScheme::wrapDegrees(int degrees) const
{
    if (degrees < 0) {
        return degrees % 360 + 360;
    } else if (degrees >= 360) {
        return degrees % 360;
    } else {
        return degrees;
    }
}

double ColorScheme::wrapDegreesDouble(double degrees)
{
    if (degrees < 0) {
        return std::fmod(degrees, 360) + 360;
    } else if (degrees >= 360) {
        return std::fmod(degrees, 360);
    } else {
        return degrees;
    }
}

double ColorScheme::hueDiff(double a, double b) const
{
    return 180.0 - std::abs(std::abs(a - b) - 180.0);
}

std::string ColorScheme::stringForColor(QRgb color) const
{
    [[maybe_unused]] constexpr int width = 4;
    const auto &hct = Cam::fromInt(color);
    const std::string h = "H" + std::to_string(std::lround(hct.getHue()));
    const std::string c = "C" + std::to_string(std::lround(hct.getChroma()));
    const std::string t = "T" + std::to_string(std::lround(CamUtils::lstarFromInt(color)));
    const std::string hex = ""; // std::format("{:x}", color & 0xffffff); TODO enable the code in C++20
    return h + c + t + " = #" + hex;
}

std::string ColorScheme::humanReadable(const std::string &paletteName, const std::array<QRgb, 12> &colors) const
{
    std::string colorsMapStr;
    for (const QRgb rgb : colors) {
        colorsMapStr.append(stringForColor(rgb) + "\n");
    }

    return paletteName + "\n" + colorsMapStr;
}

double ColorScheme::score(const Cam &cam, double proportion) const
{
    const double proportionScore = 0.7 * 100.0 * proportion;
    const double chromaScore = cam.getChroma() < ACCENT1_CHROMA ? 0.1 * (cam.getChroma() - ACCENT1_CHROMA) : 0.3 * (cam.getChroma() - ACCENT1_CHROMA);
    return chromaScore + proportionScore;
}

std::array<double, 360>
ColorScheme::getHuePopulations(const std::map<QRgb, Cam> &camByColor, const std::map<QRgb, double> &populationByColor, bool filter) const
{
    std::array<double, 360> huePopulation;
    huePopulation.fill(0.0);

    for (const auto &entry : populationByColor) {
        const double population = populationByColor.at(entry.first);
        const auto &cam = camByColor.at(entry.first);
        // TODO upstream
        if (filter && cam.getChroma() <= MIN_CHROMA) {
            continue;
        }
        const int hue = std::lround(cam.getHue()) % 360;
        huePopulation[hue] += +population;
    }

    return huePopulation;
}

void ColorScheme::init()
{
    const auto &proposedSeedCam = Cam::fromInt(m_seed);
    QRgb seedArgb;
    if (m_style != Style::CONTENT && proposedSeedCam.getChroma() < MIN_CHROMA) {
        seedArgb = PLASMA_BLUE;
    } else {
        seedArgb = m_seed;
    }
    const Cam &camSeed = Cam::fromInt(seedArgb);
    const auto &coreSpec = style(m_style);
    m_accent1 = coreSpec.a1.shades(camSeed);
    m_accent2 = coreSpec.a2.shades(camSeed);
    m_accent3 = coreSpec.a3.shades(camSeed);
    m_neutral1 = coreSpec.n1.shades(camSeed);
    m_neutral2 = coreSpec.n2.shades(camSeed);
}
