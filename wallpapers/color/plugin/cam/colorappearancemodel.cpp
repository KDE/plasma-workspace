/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "colorappearancemodel.h"

#include <cmath>

#include "frame.h"
#include "hctsolver.h"

// The maximum difference between the requested L* and the L* returned.
#define DL_MAX 0.2;
// The maximum color distance, in CAM16-UCS, between a requested color and the color returned.
#define DE_MAX 1.0;
// When the delta between the floor & ceiling of a binary search for chroma is less than this,
// the binary search terminates.
#define CHROMA_SEARCH_ENDPOINT 0.4;
// When the delta between the floor & ceiling of a binary search for J, lightness in CAM16,
// is less than this, the binary search terminates.
#define LIGHTNESS_SEARCH_ENDPOINT 0.01;

namespace CamUtils
{
double linearized(int rgbComponent)
{
    const double normalized = rgbComponent / 255.0;

    if (normalized <= 0.04045) {
        return (normalized / 12.92) * 100.0;
    } else {
        return std::pow(((normalized + 0.055) / 1.055), 2.4) * 100.0;
    }
}

int delinearized(double rgbComponent)
{
    const double normalized = rgbComponent / 100.0;
    double delinearized = 0.0;
    if (normalized <= 0.0031308) {
        delinearized = normalized * 12.92;
    } else {
        delinearized = 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
    }
    return std::clamp<int>(std::lround(delinearized * 255.0), 0, 255);
}

/**
 * Converts a color from RGB components to ARGB format.
 *
 * @note alpha is not used
 */
QRgb argbFromRgb(int red, int green, int blue)
{
    return ((red & 255) << 16) | ((green & 255) << 8) | (blue & 255);
}

int argbFromXyz(double x, double y, double z)
{
    constexpr auto &matrix = XYZ_TO_SRGB;
    const double linearR = matrix[0][0] * x + matrix[0][1] * y + matrix[0][2] * z;
    const double linearG = matrix[1][0] * x + matrix[1][1] * y + matrix[1][2] * z;
    const double linearB = matrix[2][0] * x + matrix[2][1] * y + matrix[2][2] * z;
    const int r = delinearized(linearR);
    const int g = delinearized(linearG);
    const int b = delinearized(linearB);
    return argbFromRgb(r, g, b);
}

int argbFromLstar(double lstar)
{
    const double fy = (lstar + 16.0) / 116.0;
    const double fz = fy;
    const double fx = fy;
    constexpr double kappa = 24389.0 / 27.0;
    constexpr double epsilon = 216.0 / 24389.0;
    const bool lExceedsEpsilonKappa = lstar > 8.0;
    const double y = lExceedsEpsilonKappa ? fy * fy * fy : lstar / kappa;
    const bool cubeExceedEpsilon = fy * fy * fy > epsilon;
    const double x = cubeExceedEpsilon ? fx * fx * fx : lstar / kappa;
    const double z = cubeExceedEpsilon ? fz * fz * fz : lstar / kappa;
    constexpr auto &whitePoint = WHITE_POINT_D65;
    return argbFromXyz(x * whitePoint[0], y * whitePoint[1], z * whitePoint[2]);
}

QRgb argbFromLinrgbComponents(double r, double g, double b)
{
    return argbFromRgb(delinearized(r), delinearized(g), delinearized(b));
}

double yFromLstar(double lstar)
{
    double ke = 8.0;
    if (lstar > ke) {
        return std::pow((lstar + 16.0) / 116.0, 3.0) * 100.0;
    } else {
        return lstar / (24389.0 / 27.0) * 100.0;
    }
}

double lstarFromInt(QRgb color)
{
    return lstarFromY(yFromInt(color));
}

double lstarFromY(double y)
{
    y = y / 100.0;
    const double e = 216.0 / 24389.0;
    double yIntermediate;
    if (y <= e) {
        return ((24389.0 / 27.0) * y);
    } else {
        yIntermediate = std::cbrt(y);
    }
    return 116.0 * yIntermediate - 16.0;
}

double yFromInt(QRgb color)
{
    const double r = linearized(qRed(color));
    const double g = linearized(qGreen(color));
    const double b = linearized(qBlue(color));
    constexpr auto &matrix = SRGB_TO_XYZ;
    const double y = (r * matrix[1][0]) + (g * matrix[1][1]) + (b * matrix[1][2]);
    return y;
}

}

Cam::Cam(double hue, double chroma, double j, double q, double m, double s, double jstar, double astar, double bstar)
    : m_hue(hue)
    , m_chroma(chroma)
    , m_J(j)
    , m_Q(q)
    , m_M(m)
    , m_S(s)
    , m_Jstar(jstar)
    , m_Astar(astar)
    , m_Bstar(bstar)
{
}

double Cam::operator-(const Cam &other) const
{
    return 180.0 - std::abs(std::abs(this->getHue() - other.getHue()) - 180.0);
}

double Cam::getHue() const
{
    return m_hue;
}

double Cam::getChroma() const
{
    return m_chroma;
}

double Cam::getJ() const
{
    return m_J;
}

double Cam::getQ() const
{
    return m_Q;
}

double Cam::getM() const
{
    return m_M;
}

double Cam::getS() const
{
    return m_S;
}

double Cam::getJstar() const
{
    return m_Jstar;
}

double Cam::getAstar() const
{
    return m_Astar;
}

double Cam::getBstar() const
{
    return m_Bstar;
}

Cam Cam::fromInt(QRgb color)
{
    // Transform RGB int to XYZ
    double sr = qRed(color) / 255.0;
    sr = sr < 0.04045 ? sr / 12.92 : std::pow((sr + 0.055) / 1.055, 2.4);
    double sg = qGreen(color) / 255.0;
    sg = sg < 0.04045 ? sg / 12.92 : std::pow((sg + 0.055) / 1.055, 2.4);
    double sb = qBlue(color) / 255.0;
    sb = sb < 0.04045 ? sb / 12.92 : std::pow((sb + 0.055) / 1.055, 2.4);

    const double x = 100 * (sr * 0.41233895 + sg * 0.35762064 + sb * 0.18051042);
    const double y = 100 * (sr * 0.2126 + sg * 0.7152 + sb * 0.0722);
    const double z = 100 * (sr * 0.01932141 + sg * 0.11916382 + sb * 0.95034478);
    const std::array<double, 3> xyz{x, y, z};

    // Transform XYZ to 'cone'/'rgb' responses
    constexpr double matrix[3][3]{
        {0.401288, 0.650173, -0.051461},
        {-0.250268, 1.204414, 0.045854},
        {-0.002079, 0.048952, 0.953127},
    };
    const double rT = (xyz[0] * matrix[0][0]) + (xyz[1] * matrix[0][1]) + (xyz[2] * matrix[0][2]);
    const double gT = (xyz[0] * matrix[1][0]) + (xyz[1] * matrix[1][1]) + (xyz[2] * matrix[1][2]);
    const double bT = (xyz[0] * matrix[2][0]) + (xyz[1] * matrix[2][1]) + (xyz[2] * matrix[2][2]);

    // Discount illuminant
    const auto &frame = Frame::defaultFrame();
    const auto &rgbD = frame.getRgbD();
    const double rD = rgbD[0] * rT;
    const double gD = rgbD[1] * gT;
    const double bD = rgbD[2] * bT;

    // Chromatic adaptation
    auto signum = [](double x) -> int {
        return x > 0 ? 1 : ((x < 0) ? -1 : 0);
    };

    const double rAF = (double)std::pow(frame.getFl() * std::abs(rD) / 100.0, 0.42);
    const double gAF = (double)std::pow(frame.getFl() * std::abs(gD) / 100.0, 0.42);
    const double bAF = (double)std::pow(frame.getFl() * std::abs(bD) / 100.0, 0.42);
    const double rA = signum(rD) * 400.0 * rAF / (rAF + 27.13);
    const double gA = signum(gD) * 400.0 * gAF / (gAF + 27.13);
    const double bA = signum(bD) * 400.0 * bAF / (bAF + 27.13);

    // redness-greenness
    const double a = (11.0 * rA + -12.0 * gA + bA) / 11.0;
    // yellowness-blueness
    const double b = (rA + gA - 2.0 * bA) / 9.0;

    // auxiliary components
    const double u = (20.0 * rA + 20.0 * gA + 21.0 * bA) / 20.0;
    const double p2 = (40.0 * rA + 20.0 * gA + bA) / 20.0;

    // hue
    const double atan2 = std::atan2(b, a);
    const double atanDegrees = atan2 * 180.0 / M_PI;
    const double hue = atanDegrees < 0 ? atanDegrees + 360.0 : atanDegrees >= 360 ? atanDegrees - 360.0 : atanDegrees;
    const double hueRadians = hue * M_PI / 180.0;

    // achromatic response to color
    const double ac = p2 * frame.getNbb();

    // CAM16 lightness and brightness
    const double j = 100.0 * std::pow(ac / frame.getAw(), frame.getC() * frame.getZ());
    const double q = 4.0 / frame.getC() * std::sqrt(j / 100.0) * (frame.getAw() + 4.0) * frame.getFlRoot();

    // CAM16 chroma, colorfulness, and saturation.
    const double huePrime = (hue < 20.14) ? hue + 360 : hue;
    const double eHue = 0.25 * (std::cos(huePrime * M_PI / 180.0 + 2.0) + 3.8);
    const double p1 = 50000.0 / 13.0 * eHue * frame.getNc() * frame.getNcb();
    const double t = p1 * std::sqrt(a * a + b * b) / (u + 0.305);
    const double alpha = std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, frame.getN()), 0.73);
    // CAM16 chroma, colorfulness, saturation
    const double c = alpha * std::sqrt(j / 100.0);
    const double m = c * frame.getFlRoot();
    const double s = 50.0 * std::sqrt((alpha * frame.getC()) / (frame.getAw() + 4.0));

    // CAM16-UCS components
    const double jstar = (1.0 + 100.0 * 0.007) * j / (1.0 + 0.007 * j);
    const double mstar = 1.0 / 0.0228 * std::log(1.0 + 0.0228 * m);
    const double astar = mstar * std::cos(hueRadians);
    const double bstar = mstar * std::sin(hueRadians);

    return Cam(hue, c, j, q, m, s, jstar, astar, bstar);
}

QRgb Cam::getInt(double hue, double chroma, double lstar)
{
    return HctSolver::solveToInt(hue, chroma, lstar);
}
