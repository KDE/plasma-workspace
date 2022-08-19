/*
    SPDX-FileCopyrightText: 2017 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "colorutils.h"

#include <cmath>

#include "cam/colorappearancemodel.h"

#define XYZ_EPSILON 0.008856
#define XYZ_KAPPA 903.3
#define XYZ_WHITE_REFERENCE_X 95.047
#define XYZ_WHITE_REFERENCE_Y 100
#define XYZ_WHITE_REFERENCE_Z 108.883

namespace ColorUtils
{
void colorToXYZ(QRgb color, std::array<double, 3> &outXyz)
{
    RGBToXYZ(qRed(color), qGreen(color), qBlue(color), outXyz);
}

void RGBToXYZ(int r, int g, int b, std::array<double, 3> &outXyz)
{
    double sr = r / 255.0;
    sr = sr < 0.04045 ? sr / 12.92 : std::pow((sr + 0.055) / 1.055, 2.4);
    double sg = g / 255.0;
    sg = sg < 0.04045 ? sg / 12.92 : std::pow((sg + 0.055) / 1.055, 2.4);
    double sb = b / 255.0;
    sb = sb < 0.04045 ? sb / 12.92 : std::pow((sb + 0.055) / 1.055, 2.4);

    outXyz[0] = 100 * (sr * 0.4124 + sg * 0.3576 + sb * 0.1805);
    outXyz[1] = 100 * (sr * 0.2126 + sg * 0.7152 + sb * 0.0722);
    outXyz[2] = 100 * (sr * 0.0193 + sg * 0.1192 + sb * 0.9505);
}

void XYZToLAB(double x, double y, double z, std::array<double, 3> &outLab)
{
    std::array<double, 3> xyz = {x / XYZ_WHITE_REFERENCE_X, y / XYZ_WHITE_REFERENCE_Y, z / XYZ_WHITE_REFERENCE_Z};
    std::transform(xyz.cbegin(), xyz.cend(), xyz.begin(), [](double v) {
        return v > XYZ_EPSILON ? std::pow(v, 1 / 3.0) : (XYZ_KAPPA * v + 16) / 116;
    });

    outLab[0] = std::max(116 * xyz[1] - 16, 0.0);
    outLab[1] = 500 * (xyz[0] - xyz[1]);
    outLab[2] = 200 * (xyz[1] - xyz[2]);
}

void colorToLAB(QRgb color, std::array<double, 3> &outLab)
{
    RGBToLAB(qRed(color), qGreen(color), qBlue(color), outLab);
}

void RGBToLAB(int r, int g, int b, std::array<double, 3> &outLab)
{
    // First we convert RGB to XYZ
    RGBToXYZ(r, g, b, outLab);
    // outLab now contains XYZ
    XYZToLAB(outLab[0], outLab[1], outLab[2], outLab);
    // outLab now contains LAB representation
}

void LABToXYZ(double l, double a, double b, std::array<double, 3> &outXyz)
{
    const double fy = (l + 16) / 116;
    const double fx = a / 500 + fy;
    const double fz = fy - b / 200;

    double tmp = std::pow(fx, 3);
    const double xr = tmp > XYZ_EPSILON ? tmp : (116 * fx - 16) / XYZ_KAPPA;
    const double yr = l > XYZ_KAPPA * XYZ_EPSILON ? std::pow(fy, 3) : l / XYZ_KAPPA;

    tmp = std::pow(fz, 3);
    const double zr = tmp > XYZ_EPSILON ? tmp : (116 * fz - 16) / XYZ_KAPPA;

    outXyz[0] = xr * XYZ_WHITE_REFERENCE_X;
    outXyz[1] = yr * XYZ_WHITE_REFERENCE_Y;
    outXyz[2] = zr * XYZ_WHITE_REFERENCE_Z;
}

QRgb XYZToColor(double x, double y, double z)
{
    double r = (x * 3.2406 + y * -1.5372 + z * -0.4986) / 100;
    double g = (x * -0.9689 + y * 1.8758 + z * 0.0415) / 100;
    double b = (x * 0.0557 + y * -0.2040 + z * 1.0570) / 100;

    r = r > 0.0031308 ? 1.055 * std::pow(r, 1 / 2.4) - 0.055 : 12.92 * r;
    g = g > 0.0031308 ? 1.055 * std::pow(g, 1 / 2.4) - 0.055 : 12.92 * g;
    b = b > 0.0031308 ? 1.055 * std::pow(b, 1 / 2.4) - 0.055 : 12.92 * b;

    const int R = std::clamp<int>(std::lround(r * 255), 0, 255);
    const int G = std::clamp<int>(std::lround(g * 255), 0, 255);
    const int B = std::clamp<int>(std::lround(b * 255), 0, 255);

    return ((R & 0x0ff) << 16) | ((G & 0x0ff) << 8) | (B & 0x0ff);
}

QRgb LABToColor(double l, double a, double b)
{
    std::array<double, 3> result;
    LABToXYZ(l, a, b, result);
    return XYZToColor(result[0], result[1], result[2]);
}

double calculateLuminance(QRgb color)
{
    std::array<double, 3> result;
    colorToXYZ(color, result);
    // Luminance is the Y component
    return result[1] / 100.0;
}

double calculateContrast(QRgb foreground, QRgb background)
{
    const double luminance1 = calculateLuminance(foreground) + 0.05;
    const double luminance2 = calculateLuminance(background) + 0.05;

    // Now return the lighter luminance divided by the darker luminance
    return std::max(luminance1, luminance2) / std::min(luminance1, luminance2);
}

}
