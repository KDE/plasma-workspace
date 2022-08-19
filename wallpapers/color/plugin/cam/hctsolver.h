/*
    SPDX-FileCopyrightText: 2022 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef HCTSOLVER_H
#define HCTSOLVER_H

#include <array>

#include "colorappearancemodel.h"

/**
 * An efficient algorithm for determining the closest sRGB color to a set of HCT coordinates,
 * based on geometrical insights for finding intersections in linear RGB, CAM16, and L*a*b*.
 *
 * Algorithm identified and implemented by Tianguang Zhang.
 * Copied from //java/com/google/ux/material/libmonet/hct on May 22 2022.
 * ColorUtils/MathUtils functions that were required were added to CamUtils.
 */
namespace HctSolver
{
QRgb solveToInt(double hueDegrees, double chroma, double lstar);
}

#endif // HCTSOLVER_H
