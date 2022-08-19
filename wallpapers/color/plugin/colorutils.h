/*
    SPDX-FileCopyrightText: 2017 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef COLORUTILS_H
#define COLORUTILS_H

#include <array>

#include <QRgb>

/**
 * A set of color-related utility methods.
 *
 * @see platform_frameworks_base/base/core/java/com/android/internal/graphics/ColorUtils.java
 */
namespace ColorUtils
{
/**
 * Convert the ARGB color to its CIE XYZ representative components.
 */
void colorToXYZ(QRgb color, std::array<double, 3> &outXyz);

/**
 * Convert RGB components to its CIE XYZ representative components.
 */
void RGBToXYZ(int r, int g, int b, std::array<double, 3> &outXyz);

/**
 * Converts a color from CIE XYZ to CIE Lab representation.
 */
void XYZToLAB(double x, double y, double z, std::array<double, 3> &outLab);

/**
 * Convert RGB components to its CIE Lab representative components.
 */
void colorToLAB(QRgb color, std::array<double, 3> &outLab);

/**
 * Convert RGB components to its CIE Lab representative components.
 */
void RGBToLAB(int r, int g, int b, std::array<double, 3> &outLab);

/**
 * Converts a color from CIE Lab to CIE XYZ representation.
 */
void LABToXYZ(double l, double a, double b, std::array<double, 3> &outXyz);

/**
 * Converts a color from CIE XYZ to its RGB representation.
 */
QRgb XYZToColor(double x, double y, double z);

/**
 * Converts a color from CIE Lab to its RGB representation.
 */
QRgb LABToColor(double l, double a, double b);

/**
 * Returns the luminance of a color as a float between 0.0 and 1.0
 */
double calculateLuminance(QRgb color);

/**
 * Returns the contrast ratio between @p foreground and @p background.
 */
double calculateContrast(QRgb foreground, QRgb background);

}

#endif // COLORUTILS_H
