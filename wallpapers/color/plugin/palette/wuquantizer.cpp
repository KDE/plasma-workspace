/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "wuquantizer.h"

#include <unordered_set>

void WuQuantizer::quantize(const std::vector<QRgb> &pixels, int colorCount)
{
    Q_ASSERT(pixels.size() > 0);

    QuantizerMap quantizerMap;
    quantizerMap.quantize(pixels, colorCount);
    m_inputPixelToCount = quantizerMap.inputPixelToCount();
    // Extraction should not be run on using a color count higher than the number of colors
    // in the pixels. The algorithm doesn't expect that to be the case, unexpected results and
    // exceptions may occur.
    const auto &mapColors = quantizerMap.getQuantizedColors();
    const std::unordered_set<QRgb> uniqueColors(mapColors.cbegin(), mapColors.cend());
    m_palette.clear();
    if (uniqueColors.size() <= static_cast<std::size_t>(colorCount)) {
        m_palette.resize(uniqueColors.size(), 0);
        int index = 0;
        for (const QRgb color : std::as_const(uniqueColors)) {
            m_palette[index++] = color;
        }
    } else {
        constructHistogram(m_inputPixelToCount);
        createMoments();
        const CreateBoxesResult &createBoxesResult = createBoxes(colorCount);
        m_palette = createResult(createBoxesResult.m_resultCount);
    }
}

int WuQuantizer::getIndex(int r, int g, int b) const
{
    return (r << 10) + (r << 6) + (g << 5) + r + g + b;
}

void WuQuantizer::constructHistogram(const std::map<QRgb, unsigned> &pixels)
{
    m_weights.fill(0);
    m_momentsR.fill(0);
    m_momentsG.fill(0);
    m_momentsB.fill(0);
    m_moments.fill(0.0);

    for (const auto &[pixel, count] : pixels) {
        const int red = qRed(pixel);
        const int green = qGreen(pixel);
        const int blue = qBlue(pixel);
        constexpr int bitsToRemove = 8 - s_bits;
        const int iR = (red >> bitsToRemove) + 1;
        const int iG = (green >> bitsToRemove) + 1;
        const int iB = (blue >> bitsToRemove) + 1;
        const int index = getIndex(iR, iG, iB);
        m_weights[index] += count;
        m_momentsR[index] += (red * count);
        m_momentsG[index] += (green * count);
        m_momentsB[index] += (blue * count);
        m_moments[index] += (count * ((red * red) + (green * green) + (blue * blue)));
    }
}

void WuQuantizer::createMoments()
{
    for (int r = 1; r < s_sideLength; ++r) {
        std::array<int, s_sideLength> area, areaR, areaG, areaB;
        std::array<double, s_sideLength> area2;
        area.fill(0);
        areaR.fill(0);
        areaG.fill(0);
        areaB.fill(0);
        area2.fill(0.0);

        for (int g = 1; g < s_sideLength; ++g) {
            int line = 0;
            int lineR = 0;
            int lineG = 0;
            int lineB = 0;

            double line2 = 0.0;
            for (int b = 1; b < s_sideLength; ++b) {
                const int index = getIndex(r, g, b);
                line += m_weights[index];
                lineR += m_momentsR[index];
                lineG += m_momentsG[index];
                lineB += m_momentsB[index];
                line2 += m_moments[index];

                area[b] += line;
                areaR[b] += lineR;
                areaG[b] += lineG;
                areaB[b] += lineB;
                area2[b] += line2;

                const int previousIndex = getIndex(r - 1, g, b);
                m_weights[index] = m_weights[previousIndex] + area[b];
                m_momentsR[index] = m_momentsR[previousIndex] + areaR[b];
                m_momentsG[index] = m_momentsG[previousIndex] + areaG[b];
                m_momentsB[index] = m_momentsB[previousIndex] + areaB[b];
                m_moments[index] = m_moments[previousIndex] + area2[b];
            }
        }
    }
}

WuQuantizer::CreateBoxesResult WuQuantizer::createBoxes(int maxColorCount)
{
    m_cubes.clear();
    m_cubes.resize(maxColorCount);
    std::vector<double> volumeVariance(maxColorCount, 0.0);
    Box &firstBox = m_cubes[0];
    firstBox.r1 = s_maxIndex;
    firstBox.g1 = s_maxIndex;
    firstBox.b1 = s_maxIndex;

    int generatedColorCount = 0;
    int next = 0;

    int i = 1;
    while (i < maxColorCount) {
        if (cut(m_cubes[next], m_cubes[i])) {
            volumeVariance[next] = (m_cubes[next].vol > 1) ? variance(m_cubes[next]) : 0.0;
            volumeVariance[i] = (m_cubes[i].vol > 1) ? variance(m_cubes[i]) : 0.0;
        } else {
            volumeVariance[next] = 0.0;
            i--;
        }

        next = 0;

        double temp = volumeVariance[0];
        for (int k = 1; k <= i; k++) {
            if (volumeVariance[k] > temp) {
                temp = volumeVariance[k];
                next = k;
            }
        }
        generatedColorCount = i + 1;
        if (temp <= 0.0) {
            break;
        }
        i++;
    }

    return CreateBoxesResult{maxColorCount, generatedColorCount};
}

std::vector<QRgb> WuQuantizer::createResult(int colorCount) const
{
    std::vector<QRgb> colors;
    colors.reserve(colorCount);

    for (int i = 0; i < colorCount; ++i) {
        const Box &cube = m_cubes.at(i);
        const int weight = volume(cube, m_weights);
        if (weight > 0) {
            const int r = (volume(cube, m_momentsR) / weight);
            const int g = (volume(cube, m_momentsG) / weight);
            const int b = (volume(cube, m_momentsB) / weight);
            const QRgb color = qRgb(r, g, b);
            colors.emplace_back(color);
        }
    }

    return colors;
}

double WuQuantizer::variance(const WuQuantizer::Box &cube) const
{
    const int dr = volume(cube, m_momentsR);
    const int dg = volume(cube, m_momentsG);
    const int db = volume(cube, m_momentsB);
    const double xx = m_moments[getIndex(cube.r1, cube.g1, cube.b1)] - m_moments[getIndex(cube.r1, cube.g1, cube.b0)]
        - m_moments[getIndex(cube.r1, cube.g0, cube.b1)] + m_moments[getIndex(cube.r1, cube.g0, cube.b0)] - m_moments[getIndex(cube.r0, cube.g1, cube.b1)]
        + m_moments[getIndex(cube.r0, cube.g1, cube.b0)] + m_moments[getIndex(cube.r0, cube.g0, cube.b1)] - m_moments[getIndex(cube.r0, cube.g0, cube.b0)];

    const int hypotenuse = dr * dr + dg * dg + db * db;
    const int volume2 = volume(cube, m_weights);
    const double variance2 = xx - ((double)hypotenuse / (double)volume2);

    return variance2;
}

bool WuQuantizer::cut(WuQuantizer::Box &one, WuQuantizer::Box &two) const
{
    const int wholeR = volume(one, m_momentsR);
    const int wholeG = volume(one, m_momentsG);
    const int wholeB = volume(one, m_momentsB);
    const int wholeW = volume(one, m_weights);

    const MaximizeResult &maxRResult = maximize(one, Direction::Red, one.r0 + 1, one.r1, wholeR, wholeG, wholeB, wholeW);
    const MaximizeResult &maxGResult = maximize(one, Direction::Green, one.g0 + 1, one.g1, wholeR, wholeG, wholeB, wholeW);
    const MaximizeResult &maxBResult = maximize(one, Direction::Blue, one.b0 + 1, one.b1, wholeR, wholeG, wholeB, wholeW);
    Direction cutDirection;
    const qreal maxR = maxRResult.m_maximum;
    const qreal maxG = maxGResult.m_maximum;
    const qreal maxB = maxBResult.m_maximum;
    if (maxR >= maxG && maxR >= maxB) {
        if (maxRResult.m_cutLocation < 0) {
            return false;
        }
        cutDirection = Direction::Red;
    } else if (maxG >= maxR && maxG >= maxB) {
        cutDirection = Direction::Green;
    } else {
        cutDirection = Direction::Blue;
    }

    two.r1 = one.r1;
    two.g1 = one.g1;
    two.b1 = one.b1;

    switch (cutDirection) {
    case Direction::Red:
        one.r1 = maxRResult.m_cutLocation;
        two.r0 = one.r1;
        two.g0 = one.g0;
        two.b0 = one.b0;
        break;
    case Direction::Green:
        one.g1 = maxGResult.m_cutLocation;
        two.r0 = one.r0;
        two.g0 = one.g1;
        two.b0 = one.b0;
        break;
    case Direction::Blue:
        one.b1 = maxBResult.m_cutLocation;
        two.r0 = one.r0;
        two.g0 = one.g0;
        two.b0 = one.b1;
        break;
    default:
        Q_UNREACHABLE();
    }

    one.vol = (one.r1 - one.r0) * (one.g1 - one.g0) * (one.b1 - one.b0);
    two.vol = (two.r1 - two.r0) * (two.g1 - two.g0) * (two.b1 - two.b0);

    return true;
}

WuQuantizer::MaximizeResult
WuQuantizer::maximize(const WuQuantizer::Box &cube, WuQuantizer::Direction direction, int first, int last, int wholeR, int wholeG, int wholeB, int wholeW) const
{
    const int baseR = bottom(cube, direction, m_momentsR);
    const int baseG = bottom(cube, direction, m_momentsG);
    const int baseB = bottom(cube, direction, m_momentsB);
    const int baseW = bottom(cube, direction, m_weights);

    qreal max = 0.0;
    int cut = -1;
    for (int i = first; i < last; i++) {
        int halfR = baseR + top(cube, direction, i, m_momentsR);
        int halfG = baseG + top(cube, direction, i, m_momentsG);
        int halfB = baseB + top(cube, direction, i, m_momentsB);
        int halfW = baseW + top(cube, direction, i, m_weights);

        if (halfW == 0) {
            continue;
        }
        qreal tempNumerator = halfR * halfR + halfG * halfG + halfB * halfB;
        qreal tempDenominator = halfW;
        qreal temp = tempNumerator / tempDenominator;

        halfR = wholeR - halfR;
        halfG = wholeG - halfG;
        halfB = wholeB - halfB;
        halfW = wholeW - halfW;
        if (halfW == 0) {
            continue;
        }

        tempNumerator = halfR * halfR + halfG * halfG + halfB * halfB;
        tempDenominator = halfW;
        temp += (tempNumerator / tempDenominator);
        if (temp > max) {
            max = temp;
            cut = i;
        }
    }

    return MaximizeResult{cut, max};
}

int WuQuantizer::volume(const WuQuantizer::Box &cube, const std::array<int, s_totalSize> &moment) const
{
    return (moment[getIndex(cube.r1, cube.g1, cube.b1)] - moment[getIndex(cube.r1, cube.g1, cube.b0)] - moment[getIndex(cube.r1, cube.g0, cube.b1)]
            + moment[getIndex(cube.r1, cube.g0, cube.b0)] - moment[getIndex(cube.r0, cube.g1, cube.b1)] + moment[getIndex(cube.r0, cube.g1, cube.b0)]
            + moment[getIndex(cube.r0, cube.g0, cube.b1)] - moment[getIndex(cube.r0, cube.g0, cube.b0)]);
}

int WuQuantizer::bottom(const WuQuantizer::Box &cube, WuQuantizer::Direction direction, const std::array<int, s_totalSize> &moment) const
{
    switch (direction) {
    case Direction::Red:
        return -moment[getIndex(cube.r0, cube.g1, cube.b1)] + moment[getIndex(cube.r0, cube.g1, cube.b0)] + moment[getIndex(cube.r0, cube.g0, cube.b1)]
            - moment[getIndex(cube.r0, cube.g0, cube.b0)];
    case Direction::Green:
        return -moment[getIndex(cube.r1, cube.g0, cube.b1)] + moment[getIndex(cube.r1, cube.g0, cube.b0)] + moment[getIndex(cube.r0, cube.g0, cube.b1)]
            - moment[getIndex(cube.r0, cube.g0, cube.b0)];
    case Direction::Blue:
        return -moment[getIndex(cube.r1, cube.g1, cube.b0)] + moment[getIndex(cube.r1, cube.g0, cube.b0)] + moment[getIndex(cube.r0, cube.g1, cube.b0)]
            - moment[getIndex(cube.r0, cube.g0, cube.b0)];
    default:
        Q_UNREACHABLE();
    }
}

int WuQuantizer::top(const WuQuantizer::Box &cube, WuQuantizer::Direction direction, int position, const std::array<int, s_totalSize> &moment) const
{
    switch (direction) {
    case Direction::Red:
        return (moment[getIndex(position, cube.g1, cube.b1)] - moment[getIndex(position, cube.g1, cube.b0)] - moment[getIndex(position, cube.g0, cube.b1)]
                + moment[getIndex(position, cube.g0, cube.b0)]);
    case Direction::Green:
        return (moment[getIndex(cube.r1, position, cube.b1)] - moment[getIndex(cube.r1, position, cube.b0)] - moment[getIndex(cube.r0, position, cube.b1)]
                + moment[getIndex(cube.r0, position, cube.b0)]);
    case Direction::Blue:
        return (moment[getIndex(cube.r1, cube.g1, position)] - moment[getIndex(cube.r1, cube.g0, position)] - moment[getIndex(cube.r0, cube.g1, position)]
                + moment[getIndex(cube.r0, cube.g0, position)]);
    default:
        Q_UNREACHABLE();
    }
}
