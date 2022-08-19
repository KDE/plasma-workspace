/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef CELEBIQUANTIZER_H
#define CELEBIQUANTIZER_H

#include "quantizer.h"

/**
 * An implementation of Celebi's quantization method.
 * See Celebi 2011, “Improving the Performance of K-Means for Color Quantization”
 *
 * First, Wu's quantizer runs. The results are used as starting points for a subsequent Kmeans
 * run. Using Wu's quantizer ensures 100% reproducible quantization results, because the starting
 * centroids are always the same. It also ensures high quality results, Wu is a box-cutting
 * quantization algorithm, much like medican color cut. It minimizes variance, much like Kmeans.
 * Wu is shown to be the highest quality box-cutting quantization algorithm.
 *
 * Second, a Kmeans quantizer tweaked for performance is run. Celebi calls this a weighted
 * square means quantizer, or WSMeans. Optimizations include operating on a map of image pixels
 * rather than all image pixels, and avoiding excess color distance calculations by using a
 * matrix and geometrical properties to know when there won't be any cluster closer to a pixel.
 *
 * @see platform_frameworks_base/base/core/java/com/android/internal/graphics/palette/CelebiQuantizer.java
 */
class CelebiQuantizer : public Quantizer
{
public:
    explicit CelebiQuantizer();
    ~CelebiQuantizer() override = default;

    void quantize(const std::vector<QRgb> &pixels, int maxColors) override;
};

#endif // CELEBIQUANTIZER_H
