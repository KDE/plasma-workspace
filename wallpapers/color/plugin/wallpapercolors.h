/*
    SPDX-FileCopyrightText: 2017 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef WALLPAPERCOLORS_H
#define WALLPAPERCOLORS_H

#include <map>
#include <optional>
#include <vector>

#include <QRgb>

#include "cam/colorappearancemodel.h"

/**
 * Provides information about the colors of a wallpaper.
 *
 * Exposes the 3 most visually representative colors of a wallpaper.
 *
 * @see platform_frameworks_base/base/core/java/android/app/WallpaperColors.java
 */
class WallpaperColors
{
public:
    /**
     * Constructs a new object from a set of colors, where hints can be specified.
     *
     * @param colorToPopulation Map with keys of colors, and value representing the number of
     *                          occurrences of color in the wallpaper.
     * @param colorHints        A combination of color hints.
     */
    explicit WallpaperColors(const std::map<QRgb, unsigned> &colorToPopulation, const std::vector<QRgb> &palette);

    /**
     * Constructs @c WallpaperColors from a bitmap.
     *
     * Main colors will be extracted from the bitmap.
     *
     * @param pixels Source where to extract from.
     */
    static WallpaperColors fromBitmap(const std::vector<QRgb> &pixels);

    /**
     * Gets the most visually representative color of the wallpaper.
     * "Visually representative" means easily noticeable in the image,
     * probably happening at high frequency.
     *
     * @return A color.
     */
    QRgb getPrimaryColor() const;

    /**
     * Gets the second most preeminent color of the wallpaper. Can be null.
     *
     * @return A color, may be null.
     */
    std::optional<QRgb> getSecondaryColor() const;

    /**
     * Gets the third most preeminent color of the wallpaper. Can be null.
     *
     * @return A color, may be null.
     */
    std::optional<QRgb> getTertiaryColor() const;

    /**
     * List of most preeminent colors, sorted by importance.
     *
     * @return List of colors.
     * @hide
     */
    const std::vector<QRgb> &getMainColors() const;

    bool operator=(const WallpaperColors &other) const;

private:
    static std::array<double, 360> getHueProportions(const std::map<QRgb, Cam> &colorToCam, const std::map<QRgb, unsigned> &colorToPopulation);
    static std::map<QRgb, double>
    getColorToHueProportion(const std::vector<QRgb> &colors, std::map<QRgb, Cam> &colorToCam, const std::array<double, 360> &hueProportions);
    static double getScore(const Cam &cam, double proportion);
    static int wrapDegrees(int degrees);

    std::vector<QRgb> m_mainColors;
    std::map<QRgb, unsigned> m_allColors;
};

#endif // WALLPAPERCOLORS_H
