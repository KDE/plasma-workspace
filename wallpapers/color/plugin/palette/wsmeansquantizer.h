/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef WSMEANSQUANTIZER_H
#define WSMEANSQUANTIZER_H

#include "quantizer.h"

constexpr bool s_debug = false;
constexpr int s_maxIterations = 10;
// Points won't be moved to a closer cluster, if the closer cluster is within
// this distance. 3.0 used because L*a*b* delta E < 3 is considered imperceptible.
constexpr float s_minMovementDistance = 3.0;

/**
 * A color quantizer based on the Kmeans algorithm. Prefer using QuantizerCelebi.
 *
 * This is an implementation of Kmeans based on Celebi's 2011 paper,
 * "Improving the Performance of K-Means for Color Quantization". In the paper, this algorithm is
 * referred to as "WSMeans", or, "Weighted Square Means" The main advantages of this Kmeans
 * implementation are taking advantage of triangle properties to avoid distance calculations, as
 * well as indexing colors by their count, thus minimizing the number of points to move around.
 *
 * Celebi's paper also stabilizes results and guarantees high quality by using starting centroids
 * from Wu's quantization algorithm. See QuantizerCelebi for more info.
 */
class WSMeansQuantizer : public Quantizer
{
public:
    explicit WSMeansQuantizer(const std::vector<QRgb> &inClusters, const std::map<QRgb, unsigned> &inputPixelToCount);

    void quantize(const std::vector<QRgb> &pixels, int maxColors) override;

private:
    struct Distance {
        std::size_t m_index;
        qreal m_distance;
    };

    void initializeClusters(std::size_t maxColors);
    void calculateClusterDistances(std::size_t maxColors);
    bool reassignPoints(std::size_t maxColors);
    void recalculateClusterCenters(std::size_t maxColors);

    std::vector<std::array<qreal, 3>> m_clusters;
    std::vector<int> m_clusterPopulations;
    decltype(m_clusters) m_points;
    std::vector<QRgb> m_pixels;
    std::vector<int> m_clusterIndices;
    std::vector<std::vector<int>> m_indexMatrix;
    std::vector<std::vector<qreal>> m_distanceMatrix;
};

#endif // WSMEANSQUANTIZER_H
