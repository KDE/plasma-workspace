/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "celebiquantizer.h"

#include "wsmeansquantizer.h"
#include "wuquantizer.h"

CelebiQuantizer::CelebiQuantizer()
    : Quantizer()
{
}

void CelebiQuantizer::quantize(const std::vector<QRgb> &pixels, int maxColors)
{
    WuQuantizer wu;
    wu.quantize(pixels, maxColors);
    WSMeansQuantizer kmeans(wu.getQuantizedColors(), wu.inputPixelToCount());
    kmeans.quantize(pixels, maxColors);
    m_palette = kmeans.getQuantizedColors();
    m_inputPixelToCount = kmeans.inputPixelToCount();
}
