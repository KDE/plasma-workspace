/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef COLORAPPEARANCEMODEL_H
#define COLORAPPEARANCEMODEL_H

#include <array>

#include <QRgb>

namespace CamUtils
{
double yFromLstar(double lstar);

constexpr std::array<double, 3> WHITE_POINT_D65{95.047, 100.0, 108.883};

constexpr double XYZToCAM16RGB[3][3]{
    {0.401288, 0.650173, -0.051461},
    {-0.250268, 1.204414, 0.045854},
    {-0.002079, 0.048952, 0.953127},
};

}

/**
 * A color appearance model, based on CAM16, extended to use L* as the lightness dimension, and
 * coupled to a gamut mapping algorithm. Creates a color system, enables a digital design system.
 *
 * @see platform_frameworks_base/base/core/java/com/android/internal/graphics/cam/Cam.java
 */
class Cam
{
public:
    explicit Cam() = default;

    explicit Cam(double hue, double chroma);

    double operator-(const Cam &other) const;

    /** Hue in CAM16 */
    double getHue() const;

    /** Chroma in CAM16 */
    double getChroma() const;

    /**
     * Create a color appearance model from a ARGB integer representing a color. It is assumed the
     * color was viewed in the frame defined in the sRGB standard.
     */
    static Cam fromInt(QRgb color);

private:
    double m_hue;
    double m_chroma;
};

#endif // COLORAPPEARANCEMODEL_H
