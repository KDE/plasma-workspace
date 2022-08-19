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
QRgb argbFromLinrgbComponents(double r, double g, double b);
int argbFromLstar(double lstar);
double yFromLstar(double lstar);
double lstarFromInt(QRgb color);
double lstarFromY(double y);
double yFromInt(QRgb color);

constexpr std::array<double, 3> WHITE_POINT_D65{95.047, 100.0, 108.883};

// This is a more precise sRGB to XYZ transformation matrix than traditionally
// used. It was derived using Schlomer's technique of transforming the xyY
// primaries to XYZ, then applying a correction to ensure mapping from sRGB
// 1, 1, 1 to the reference white point, D65.
constexpr double SRGB_TO_XYZ[3][3] = {
    {0.41233895, 0.35762064, 0.18051042},
    {0.2126, 0.7152, 0.0722},
    {0.01932141, 0.11916382, 0.95034478},
};

constexpr double XYZ_TO_SRGB[3][3]{
    {
        3.2413774792388685,
        -1.5376652402851851,
        -0.49885366846268053,
    },
    {
        -0.9691452513005321,
        1.8758853451067872,
        0.04156585616912061,
    },
    {
        0.05562093689691305,
        -0.20395524564742123,
        1.0571799111220335,
    },
};
constexpr double XYZToCAM16RGB[3][3]{
    {0.401288, 0.650173, -0.051461},
    {-0.250268, 1.204414, 0.045854},
    {-0.002079, 0.048952, 0.953127},
};

}

/**
 * A color appearance model, based on CAM16, extended to use L* as the lightness dimension, and
 * coupled to a gamut mapping algorithm. Creates a color system, enables a digital design system.
 */
class Cam
{
public:
    explicit Cam() = default;

    explicit Cam(double hue, double chroma, double j, double q, double m, double s, double jstar, double astar, double bstar);

    double operator-(const Cam &other) const;

    /** Hue in CAM16 */
    double getHue() const;

    /** Chroma in CAM16 */
    double getChroma() const;

    /** Lightness in CAM16 */
    double getJ() const;

    /**
     * Brightness in CAM16.
     *
     * @note Prefer lightness, brightness is an absolute quantity. For example, a sheet of white paper
     * is much brighter viewed in sunlight than in indoor light, but it is the lightest object under
     * any lighting.
     */
    double getQ() const;

    /**
     * Colorfulness in CAM16.
     *
     * @note Prefer chroma, colorfulness is an absolute quantity. For example, a yellow toy car is much
     * more colorful outside than inside, but it has the same chroma in both environments.
     */
    double getM() const;

    /**
     * Saturation in CAM16.
     *
     * @note Colorfulness in proportion to brightness. Prefer chroma, saturation measures colorfulness
     * relative to the color's own brightness, where chroma is colorfulness relative to white.
     */
    double getS() const;

    /** Lightness coordinate in CAM16-UCS */
    double getJstar() const;

    /** a* coordinate in CAM16-UCS */
    double getAstar() const;

    /** b* coordinate in CAM16-UCS */
    double getBstar() const;

    /**
     * Create a color appearance model from a ARGB integer representing a color. It is assumed the
     * color was viewed in the frame defined in the sRGB standard.
     */
    static Cam fromInt(QRgb color);

    /**
     * Given a hue & chroma in CAM16, L* in L*a*b*, return an ARGB integer. The chroma of the color
     * returned may, and frequently will, be lower than requested. Assumes the color is viewed in
     * the
     * frame defined by the sRGB standard.
     */
    static QRgb getInt(double hue, double chroma, double lstar);

private:
    // CAM16 color dimensions, see getters for documentation.
    double m_hue;
    double m_chroma;
    double m_J;
    double m_Q;
    double m_M;
    double m_S;

    // Coordinates in UCS space. Used to determine color distance, like delta E equations in L*a*b*.
    double m_Jstar;
    double m_Astar;
    double m_Bstar;
};

#endif // COLORAPPEARANCEMODEL_H
