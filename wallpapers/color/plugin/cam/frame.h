/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef FRAME_H
#define FRAME_H

#include <array>

/**
 * The frame, or viewing conditions, where a color was seen. Used, along with a color, to create a
 * color appearance model representing the color.
 *
 * To convert a traditional color to a color appearance model, it requires knowing what
 * conditions the color was observed in. Our perception of color depends on, for example, the tone
 * of the light illuminating the color, how bright that light was, etc.
 *
 * This class is modelled separately from the color appearance model itself because there are a
 * number of calculations during the color => CAM conversion process that depend only on the viewing
 * conditions. Caching those calculations in a Frame instance saves a significant amount of time.
 */
class Frame
{
    // Standard viewing conditions assumed in RGB specification - Stokes, Anderson, Chandrasekar,
    // Motta - A Standard Default Color Space for the Internet: sRGB, 1996.
    //
    // White point = D65
    // Luminance of adapting field: 200 / Pi / 5, units are cd/m^2.
    //   sRGB ambient illuminance = 64 lux (per sRGB spec). However, the spec notes this is
    //     artificially low and based on monitors in 1990s. Use 200, the sRGB spec says this is the
    //     real average, and a survey of lux values on Wikipedia confirms this is a comfortable
    //     default: somewhere between a very dark overcast day and office lighting.
    //   Per CAM16 introduction paper (Li et al, 2017) Ew = pi * lw, and La = lw * Yb/Yw
    //   Ew = ambient environment luminance, in lux.
    //   Yb/Yw is taken to be midgray, ~20% relative luminance (XYZ Y 18.4, CIELAB L* 50).
    //   Therefore La = (Ew / pi) * .184
    //   La = 200 / pi * .184
    // Image surround to 10 degrees = ~20% relative luminance = CIELAB L* 50
    //
    // Not from sRGB standard:
    // Surround = average, 2.0.
    // Discounting illuminant = false, doesn't occur for self-luminous displays
public:
    static Frame defaultFrame();

    double getAw() const;
    double getN() const;
    double getNbb() const;
    double getNcb() const;
    double getC() const;
    double getNc() const;
    const std::array<double, 3> &getRgbD() const;
    double getFl() const;
    double getFlRoot() const;
    double getZ() const;

    /** Create a custom frame. */
    static Frame make(const std::array<double, 3> &whitepoint, double adaptingLuminance, double backgroundLstar, double surround, bool discountingIlluminant);

private:
    Frame(double n, double aw, double nbb, double ncb, double c, double nc, const std::array<double, 3> &rgbD, double fl, double fLRoot, double z);

    double m_Aw;
    double m_Nbb;
    double m_Ncb;
    double m_C;
    double m_Nc;
    double m_N;
    std::array<double, 3> m_RgbD;
    double m_Fl;
    double m_FlRoot;
    double m_Z;
};

#endif // FRAME_H
