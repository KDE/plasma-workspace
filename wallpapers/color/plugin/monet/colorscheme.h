/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef COLORSCHEME_H
#define COLORSCHEME_H

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cam/colorappearancemodel.h"

class WallpaperColors;

class Hue
{
public:
    virtual ~Hue() = default;

    virtual double get(const Cam &sourceColor) const = 0;

    /**
     * Given a hue, and a mapping of hues to hue rotations, find which hues in the mapping the
     * hue fall betweens, and use the hue rotation of the lower hue.
     *
     * @param sourceHue hue of source color
     * @param hueAndRotations list of pairs, where the first item in a pair is a hue, and the
     *    second item in the pair is a hue rotation that should be applied
     */
    virtual double getHueRotation(double sourceHue, const std::array<std::pair<int, int>, 9> &hueAndRotations) const;
};

class HueSource : public Hue
{
public:
    double get(const Cam &sourceColor) const override;
};

class HueAdd : public Hue
{
public:
    explicit HueAdd(double amountDegrees);

    double get(const Cam &sourceColor) const override;

private:
    double m_amountDegrees = 0.0;
};

class HueSubtract : public Hue
{
public:
    explicit HueSubtract(double amountDegrees);
    double get(const Cam &sourceColor) const override;

private:
    double m_amountDegrees = 0.0;
};

class HueVibrantSecondary : public Hue
{
public:
    double get(const Cam &sourceColor) const override;

private:
    static constexpr std::array<std::pair<int, int>, 9> m_hueToRotations{std::make_pair(0, 18),
                                                                         std::make_pair(41, 15),
                                                                         std::make_pair(61, 10),
                                                                         std::make_pair(101, 12),
                                                                         std::make_pair(131, 15),
                                                                         std::make_pair(181, 18),
                                                                         std::make_pair(251, 15),
                                                                         std::make_pair(301, 12),
                                                                         std::make_pair(360, 12)};
};

class HueVibrantTertiary : public Hue
{
public:
    double get(const Cam &sourceColor) const override;

private:
    static constexpr std::array<std::pair<int, int>, 9> m_hueToRotations{std::make_pair(0, 35),
                                                                         std::make_pair(41, 30),
                                                                         std::make_pair(61, 20),
                                                                         std::make_pair(101, 25),
                                                                         std::make_pair(131, 30),
                                                                         std::make_pair(181, 35),
                                                                         std::make_pair(251, 30),
                                                                         std::make_pair(301, 25),
                                                                         std::make_pair(360, 25)};
};

class HueExpressiveSecondary : public Hue
{
public:
    double get(const Cam &sourceColor) const override;

private:
    static constexpr std::array<std::pair<int, int>, 9> m_hueToRotations{std::make_pair(0, 45),
                                                                         std::make_pair(21, 95),
                                                                         std::make_pair(51, 45),
                                                                         std::make_pair(121, 20),
                                                                         std::make_pair(151, 45),
                                                                         std::make_pair(191, 90),
                                                                         std::make_pair(271, 45),
                                                                         std::make_pair(321, 45),
                                                                         std::make_pair(360, 45)};
};

class HueExpressiveTertiary : public Hue
{
public:
    double get(const Cam &sourceColor) const override;

private:
    static constexpr std::array<std::pair<int, int>, 9> m_hueToRotations{std::make_pair(0, 120),
                                                                         std::make_pair(21, 120),
                                                                         std::make_pair(51, 20),
                                                                         std::make_pair(121, 45),
                                                                         std::make_pair(151, 20),
                                                                         std::make_pair(191, 15),
                                                                         std::make_pair(271, 20),
                                                                         std::make_pair(321, 120),
                                                                         std::make_pair(360, 120)};
};

class Chroma
{
public:
    virtual ~Chroma() = default;

    virtual double get(const Cam &sourceColor) const = 0;
};

class ChromaMaxOut : public Chroma
{
    double get(const Cam &sourceColor) const override;
};

class ChromaMultiple : public Chroma
{
public:
    explicit ChromaMultiple(double multiple);

    double get(const Cam &sourceColor) const override;

private:
    double m_multiple = 1.0;
};

class ChromaConstant : public Chroma
{
public:
    explicit ChromaConstant(double chroma);

    double get(const Cam &sourceColor) const override;

private:
    double m_chroma = 0.0;
};

class ChromaSource : public ChromaMultiple
{
public:
    explicit ChromaSource();
};

class TonalSpec
{
public:
    explicit TonalSpec(Hue *hue, Chroma *chroma);

    std::array<QRgb, 12> shades(const Cam &sourceColor) const;

private:
    std::unique_ptr<Hue> m_hue;
    std::unique_ptr<Chroma> m_chroma;
};

struct CoreSpec {
    TonalSpec a1;
    TonalSpec a2;
    TonalSpec a3;
    TonalSpec n1;
    TonalSpec n2;
};

enum class Style {
    SPRITZ,
    TONAL_SPOT,
    VIBRANT,
    EXPRESSIVE,
    RAINBOW,
    FRUIT_SALAD,
    CONTENT,
    VANILLA,
};

CoreSpec style(Style s);

class ColorScheme
{
public:
    explicit ColorScheme(QRgb seed, bool darkTheme, Style style = Style::TONAL_SPOT);
    explicit ColorScheme(const WallpaperColors &wallpaperColors, bool darkTheme, Style style = Style::TONAL_SPOT);

    std::vector<QRgb> allAccentColors() const;
    std::vector<QRgb> allNeutralColors() const;

    QRgb backgroundColor() const;
    QRgb accentColor() const;

    operator std::string() const;

    /**
     * Identifies a color to create a color scheme from.
     *
     * @param wallpaperColors Colors extracted from an image via quantization.
     * @param filter If false, allow colors that have low chroma, creating grayscale themes.
     * @return ARGB int representing the color
     */
    QRgb getSeedColor(const WallpaperColors &wallpaperColors, bool filter = true) const;

    /**
     * Filters and ranks colors from WallpaperColors.
     *
     * @param wallpaperColors Colors extracted from an image via quantization.
     * @param filter If false, allow colors that have low chroma, creating grayscale themes.
     * @return List of ARGB ints, ordered from highest scoring to lowest.
     */
    std::vector<QRgb> getSeedColors(const WallpaperColors &wallpaperColors, bool filter = true) const;

    static double wrapDegreesDouble(double degrees);

private:
    int wrapDegrees(int degrees) const;
    double hueDiff(double a, double b) const;
    std::string stringForColor(QRgb color) const;
    std::string humanReadable(const std::string &paletteName, const std::array<QRgb, 12> &colors) const;
    double score(const Cam &cam, double proportion) const;
    std::array<double, 360> getHuePopulations(const std::map<QRgb, Cam> &camByColor, const std::map<QRgb, double> &populationByColor, bool filter = true) const;

    void init();

    std::array<QRgb, 12> m_accent1;
    decltype(m_accent1) m_accent2;
    decltype(m_accent1) m_accent3;
    decltype(m_accent1) m_neutral1;
    decltype(m_accent1) m_neutral2;

    QRgb m_seed = 0;
    bool m_darkTheme = false;
    Style m_style = Style::TONAL_SPOT;
};

#endif // COLORSCHEME_H
