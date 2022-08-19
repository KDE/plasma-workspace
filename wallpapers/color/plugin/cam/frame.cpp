/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "frame.h"

#include <algorithm>
#include <cmath>

#include "colorappearancemodel.h"

Frame Frame::defaultFrame()
{
    return make(CamUtils::WHITE_POINT_D65, 200.0 / M_PI * CamUtils::yFromLstar(50.0) / 100.0, 50.0, 2.0, false);
}

double Frame::getAw() const
{
    return m_Aw;
}

double Frame::getN() const
{
    return m_N;
}

double Frame::getNbb() const
{
    return m_Nbb;
}

double Frame::getNcb() const
{
    return m_Ncb;
}

double Frame::getC() const
{
    return m_C;
}

double Frame::getNc() const
{
    return m_Nc;
}

const std::array<double, 3> &Frame::getRgbD() const
{
    return m_RgbD;
}

double Frame::getFl() const
{
    return m_Fl;
}

double Frame::getFlRoot() const
{
    return m_FlRoot;
}

double Frame::getZ() const
{
    return m_Z;
}

Frame Frame::make(const std::array<double, 3> &whitepoint, double adaptingLuminance, double backgroundLstar, double surround, bool discountingIlluminant)
{
    // TODO Port to std::lerp in C++20
    auto lerp = [](double start, double stop, double amount) -> double {
        return start + (stop - start) * amount;
    };

    // Transform white point XYZ to 'cone'/'rgb' responses
    constexpr auto &matrix = CamUtils::XYZToCAM16RGB;
    const auto &xyz = whitepoint;
    const double rW = (xyz[0] * matrix[0][0]) + (xyz[1] * matrix[0][1]) + (xyz[2] * matrix[0][2]);
    const double gW = (xyz[0] * matrix[1][0]) + (xyz[1] * matrix[1][1]) + (xyz[2] * matrix[1][2]);
    const double bW = (xyz[0] * matrix[2][0]) + (xyz[1] * matrix[2][1]) + (xyz[2] * matrix[2][2]);

    // Scale input surround, domain (0, 2), to CAM16 surround, domain (0.8, 1.0)
    const double f = 0.8 + (surround / 10.0);
    // "Exponential non-linearity"
    const double c = (f >= 0.9) ? lerp(0.59, 0.69, ((f - 0.9) * 10.0)) : lerp(0.525, 0.59, ((f - 0.8) * 10.0));
    // Calculate degree of adaptation to illuminant
    double d = discountingIlluminant ? 1.0 : f * (1.0 - ((1.0 / 3.6) * std::exp((-adaptingLuminance - 42.0) / 92.0)));
    // Per Li et al, if D is greater than 1 or less than 0, set it to 1 or 0.
    d = std::clamp(d, 0.0, 1.0);
    // Chromatic induction factor
    const double nc = f;

    // Cone responses to the whitepoint, adjusted for illuminant discounting.
    //
    // Why use 100.0 instead of the white point's relative luminance?
    //
    // Some papers and implementations, for both CAM02 and CAM16, use the Y
    // value of the reference white instead of 100. Fairchild's Color Appearance
    // Models (3rd edition) notes that this is in error: it was included in the
    // CIE 2004a report on CIECAM02, but, later parts of the conversion process
    // account for scaling of appearance relative to the white point relative
    // luminance. This part should simply use 100 as luminance.
    const decltype(m_RgbD) rgbD{
        d * (100.0 / rW) + 1.0 - d,
        d * (100.0 / gW) + 1.0 - d,
        d * (100.0 / bW) + 1.0 - d,
    };
    // Luminance-level adaptation factor
    const double k = 1.0 / (5.0 * adaptingLuminance + 1.0);
    const double k4 = k * k * k * k;
    const double k4F = 1.0 - k4;
    const double fl = (k4 * adaptingLuminance) + (0.1 * k4F * k4F * std::cbrt(5.0 * adaptingLuminance));

    // Intermediate factor, ratio of background relative luminance to white relative luminance
    const double n = CamUtils::yFromLstar(backgroundLstar) / whitepoint[1];

    // Base exponential nonlinearity
    // note Schlomer 2018 has a typo and uses 1.58, the correct factor is 1.48
    const double z = 1.48 + std::sqrt(n);

    // Luminance-level induction factors
    const double nbb = 0.725 / std::pow(n, 0.2);
    const double ncb = nbb;

    // Discounted cone responses to the white point, adjusted for post-chromatic
    // adaptation perceptual nonlinearities.
    const decltype(m_RgbD) rgbAFactors{std::pow(fl * rgbD[0] * rW / 100.0, 0.42),
                                       std::pow(fl * rgbD[1] * gW / 100.0, 0.42),
                                       std::pow(fl * rgbD[2] * bW / 100.0, 0.42)};

    const decltype(m_RgbD) rgbA{
        (400.0 * rgbAFactors[0]) / (rgbAFactors[0] + 27.13),
        (400.0 * rgbAFactors[1]) / (rgbAFactors[1] + 27.13),
        (400.0 * rgbAFactors[2]) / (rgbAFactors[2] + 27.13),
    };

    const double aw = ((2.0 * rgbA[0]) + rgbA[1] + (0.05 * rgbA[2])) * nbb;

    return Frame(n, aw, nbb, ncb, c, nc, rgbD, fl, (double)std::pow(fl, 0.25), z);
}

Frame::Frame(double n, double aw, double nbb, double ncb, double c, double nc, const std::array<double, 3> &rgbD, double fl, double fLRoot, double z)
    : m_Aw(aw)
    , m_Nbb(nbb)
    , m_Ncb(ncb)
    , m_C(c)
    , m_Nc(nc)
    , m_N(n)
    , m_RgbD(rgbD)
    , m_Fl(fl)
    , m_FlRoot(fLRoot)
    , m_Z(z)
{
}
