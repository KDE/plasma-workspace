/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "wsmeansquantizer.h"

#include <cmath>
#include <random>
#include <unordered_set>

#include <QColor>

#include "colorutils.h"

/**
 *  Allows quantizers to operate in the L*a*b* colorspace.
 *  L*a*b* is a good choice for measuring distance between colors.
 *  Better spaces, and better distance calculations even in L*a*b* exist, but measuring distance
 *  in L*a*b* space, also known as deltaE, is a universally accepted standard across industries
 *  and worldwide.
 */
namespace LABPointProvider
{
std::array<double, 3> fromInt(QRgb color)
{
    std::array<double, 3> outLab;
    ColorUtils::colorToLAB(color, outLab);
    return outLab;
}

QRgb toInt(const std::array<double, 3> &lab)
{
    return ColorUtils::LABToColor(lab[0], lab[1], lab[2]);
}

double distance(const std::array<double, 3> &a, decltype(a) b)
{
    // Standard v1 CIELAB deltaE formula, 1976 - easily improved upon, however,
    // improvements do not significantly impact the Palette algorithm's results.
    const double dL = a[0] - b[0];
    const double dA = a[1] - b[1];
    const double dB = a[2] - b[2];
    return dL * dL + dA * dA + dB * dB;
}

}

WSMeansQuantizer::WSMeansQuantizer(const std::vector<QRgb> &inClusters, const std::map<QRgb, unsigned> &inputPixelToCount)
    : Quantizer()
{
    m_clusters.resize(inClusters.size());
    int index = 0;
    for (const QRgb cluster : inClusters) {
        const auto &point = LABPointProvider::fromInt(cluster);
        m_clusters[index++] = point;
    }

    m_inputPixelToCount = inputPixelToCount;
}

void WSMeansQuantizer::quantize(const std::vector<QRgb> &pixels, int maxColors)
{
    Q_ASSERT(pixels.size() > 0);

    if (m_inputPixelToCount.empty()) {
        QuantizerMap mapQuantizer;
        mapQuantizer.quantize(pixels, maxColors);
        m_inputPixelToCount = mapQuantizer.inputPixelToCount();
    }

    m_points.clear();
    m_points.resize(m_inputPixelToCount.size());
    m_pixels.clear();
    m_pixels.resize(m_inputPixelToCount.size());
    int index = 0;
    decltype(m_pixels) quantizedPixels;
    quantizedPixels.reserve(m_inputPixelToCount.size());
    for (const auto &pr : std::as_const(m_inputPixelToCount)) {
        quantizedPixels.emplace_back(pr.first);
    }

    for (const QRgb pixel : std::as_const(quantizedPixels)) {
        m_pixels[index] = pixel;
        m_points[index] = LABPointProvider::fromInt(pixel);
        index++;
    }
    if (m_clusters.size() > 0) {
        // This implies that the constructor was provided starting clusters. If that was the
        // case, we limit the number of clusters to the number of starting clusters and don't
        // initialize random clusters.
        maxColors = std::min<int>(maxColors, m_clusters.size());
    }
    maxColors = std::min<int>(maxColors, m_points.size());

    initializeClusters(maxColors);
    for (int i = 0; i < s_maxIterations; i++) {
        calculateClusterDistances(maxColors);
        if (!reassignPoints(maxColors)) {
            break;
        }
        recalculateClusterCenters(maxColors);
    }

    std::vector<std::pair<QRgb, int>> exportList;
    m_inputPixelToCount.clear();
    for (int i = 0; i < maxColors; i++) {
        const auto &cluster = m_clusters[i];
        const QRgb colorInt = LABPointProvider::toInt(cluster);
        auto pr = m_inputPixelToCount.emplace(colorInt, m_clusterPopulations[i]);
        exportList.emplace_back(*(pr.first));
    }
    std::sort(exportList.begin(), exportList.end(), [](const std::pair<QRgb, int> &a, decltype(a) b) {
        return a.second > b.second;
    });
    m_palette.clear();
    m_palette.reserve(exportList.size());
    for (const auto &pr : std::as_const(exportList)) {
        m_palette.emplace_back(pr.first);
    }
}

void WSMeansQuantizer::initializeClusters(std::size_t maxColors)
{
    std::mt19937 randomEngine(0x42688);
    const int additionalClustersNeeded = maxColors - m_clusters.size();
    if (additionalClustersNeeded > 0) {
        std::uniform_int_distribution<int> random(0, m_points.size() - 1);
        decltype(m_clusters) additionalClusters(additionalClustersNeeded);
        std::unordered_set<int> clusterIndicesUsed;
        for (int i = 0; i < additionalClustersNeeded; i++) {
            int index = random(randomEngine);
            while (clusterIndicesUsed.count(index) && clusterIndicesUsed.size() < m_points.size()) {
                index = random(randomEngine);
            }
            clusterIndicesUsed.emplace(index);
            additionalClusters.emplace_back(m_points.at(index));
        }

        decltype(m_clusters) clusters;
        clusters.reserve(maxColors);
        std::copy(m_clusters.cbegin(), m_clusters.cend(), std::back_inserter(clusters));
        std::copy(additionalClusters.cbegin(), additionalClusters.cend(), std::back_inserter(clusters));

        m_clusters = clusters;
    }

    m_clusterIndices.clear();
    m_clusterIndices.resize(m_pixels.size(), 0);
    m_clusterPopulations.clear();
    m_clusterPopulations.resize(m_pixels.size(), 0);
    std::uniform_int_distribution<int> random2(0, maxColors - 1);
    for (std::size_t i = 0; i < m_pixels.size(); i++) {
        int clusterIndex = random2(randomEngine);
        m_clusterIndices[i] = clusterIndex;
        m_clusterPopulations[i] = m_inputPixelToCount[m_pixels[i]];
    }
}

void WSMeansQuantizer::calculateClusterDistances(std::size_t maxColors)
{
    if (m_distanceMatrix.size() != maxColors) {
        m_distanceMatrix.clear();
        m_distanceMatrix.resize(maxColors);
        for (auto &matrix : m_distanceMatrix) {
            matrix.resize(maxColors, 0.0);
        }
    }

    for (std::size_t i = 0; i < maxColors; i++) {
        for (std::size_t j = i + 1; j < maxColors; j++) {
            const double distance = LABPointProvider::distance(m_clusters[i], m_clusters[j]);
            m_distanceMatrix[j][i] = distance;
            m_distanceMatrix[i][j] = distance;
        }
    }

    if (m_indexMatrix.size() != maxColors) {
        m_indexMatrix.clear();
        m_indexMatrix.resize(maxColors);
        for (auto &matrix : m_indexMatrix) {
            matrix.resize(maxColors, 0);
        }
    }

    for (std::size_t i = 0; i < maxColors; i++) {
        std::vector<Distance> distances;
        distances.reserve(maxColors);
        for (std::size_t index = 0; index < maxColors; index++) {
            distances.emplace_back(Distance{index, m_distanceMatrix[i][index]});
        }

        std::sort(distances.begin(), distances.end(), [](const Distance &a, const Distance &b) {
            return a.m_distance < b.m_distance;
        });

        for (std::size_t j = 0; j < maxColors; j++) {
            m_indexMatrix[i][j] = distances.at(j).m_index;
        }
    }
}

bool WSMeansQuantizer::reassignPoints(std::size_t maxColors)
{
    bool colorMoved = false;
    for (std::size_t i = 0; i < m_points.size(); i++) {
        const auto &point = m_points[i];
        int previousClusterIndex = m_clusterIndices[i];
        const auto &previousCluster = m_clusters[previousClusterIndex];
        const double previousDistance = LABPointProvider::distance(point, previousCluster);

        double minimumDistance = previousDistance;
        int newClusterIndex = -1;
        for (std::size_t j = 1; j < maxColors; j++) {
            int t = m_indexMatrix[previousClusterIndex][j];
            if (m_distanceMatrix[previousClusterIndex][t] >= 4 * previousDistance) {
                // Triangle inequality proves there's can be no closer center.
                break;
            }
            const double distance = LABPointProvider::distance(point, m_clusters[t]);
            if (distance < minimumDistance) {
                minimumDistance = distance;
                newClusterIndex = t;
            }
        }
        if (newClusterIndex != -1) {
            const double distanceChange = std::abs(std::sqrt(minimumDistance) - std::sqrt(previousDistance));
            if (distanceChange > s_minMovementDistance) {
                colorMoved = true;
                m_clusterIndices[i] = newClusterIndex;
            }
        }
    }
    return colorMoved;
}

void WSMeansQuantizer::recalculateClusterCenters(std::size_t maxColors)
{
    m_clusterPopulations.clear();
    m_clusterPopulations.resize(maxColors, 0);
    std::vector<double> aSums(maxColors, 0.0);
    decltype(aSums) bSums(maxColors, 0.0);
    decltype(aSums) cSums(maxColors, 0.0);
    for (std::size_t i = 0; i < m_points.size(); i++) {
        const int clusterIndex = m_clusterIndices[i];
        const auto &point = m_points[i];
        const int pixel = m_pixels[i];
        const int count = m_inputPixelToCount.at(pixel);
        m_clusterPopulations[clusterIndex] += count;
        aSums[clusterIndex] += point[0] * count;
        bSums[clusterIndex] += point[1] * count;
        cSums[clusterIndex] += point[2] * count;
    }
    for (std::size_t i = 0; i < maxColors; i++) {
        const double count = m_clusterPopulations[i];
        double aSum = aSums[i];
        double bSum = bSums[i];
        double cSum = cSums[i];
        m_clusters[i][0] = aSum / count;
        m_clusters[i][1] = bSum / count;
        m_clusters[i][2] = cSum / count;
    }
}
