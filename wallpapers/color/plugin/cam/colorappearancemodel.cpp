/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "colorappearancemodel.h"

#include <cmath>

#include "colorutils.h"
#include "frame.h"

namespace CamUtils
{
double yFromLstar(double lstar)
{
    double ke = 8.0;
    if (lstar > ke) {
        return std::pow((lstar + 16.0) / 116.0, 3.0) * 100.0;
    } else {
        return lstar / (24389.0 / 27.0) * 100.0;
    }
}

}

Cam::Cam(double hue, double chroma)
    : m_hue(hue)
    , m_chroma(chroma)
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

Cam Cam::fromInt(QRgb color)
{
    // Transform RGB int to XYZ
    std::array<double, 3> xyz;
    ColorUtils::colorToXYZ(color, xyz);

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

    // achromatic response to color
    const double ac = p2 * frame.getNbb();

    // CAM16 lightness
    const double j = 100.0 * std::pow(ac / frame.getAw(), frame.getC() * frame.getZ());

    // CAM16 chroma, colorfulness, and saturation.
    const double huePrime = (hue < 20.14) ? hue + 360 : hue;
    const double eHue = 0.25 * (std::cos(huePrime * M_PI / 180.0 + 2.0) + 3.8);
    const double p1 = 50000.0 / 13.0 * eHue * frame.getNc() * frame.getNcb();
    const double t = p1 * std::sqrt(a * a + b * b) / (u + 0.305);
    const double alpha = std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, frame.getN()), 0.73);
    // CAM16 chroma, colorfulness, saturation
    const double c = alpha * std::sqrt(j / 100.0);

    return Cam(hue, c);
}
