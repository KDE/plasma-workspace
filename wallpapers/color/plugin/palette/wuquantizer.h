/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#ifndef WUQUANTIZER_H
#define WUQUANTIZER_H

#include "quantizer.h"

/**
 * A histogram of all the input colors is constructed. It has the shape of a
 * cube. The cube would be too large if it contained all 16 million colors:
 * historical best practice is to use 5 bits  of the 8 in each channel,
 * reducing the histogram to a volume of ~32,000.
 */
constexpr int s_bits = 5;
constexpr int s_maxIndex = 32;
constexpr int s_sideLength = 33;
constexpr int s_totalSize = 35937;

/**
 * Wu's quantization algorithm is a box-cut quantizer that minimizes variance. It takes longer to
 * run than, say, median color cut, but provides the highest quality results currently known.
 *
 * Prefer `QuantizerCelebi`: coupled with Kmeans, this provides the best-known results for image
 * quantization.
 *
 * Seemingly all Wu implementations are based off of one C code snippet that cites a book from 1992
 * Graphics Gems vol. II, pp. 126-133. As a result, it is very hard to understand the mechanics of
 * the algorithm, beyond the commentary provided in the C code. Comments on the methods of this
 * class are avoided in favor of finding another implementation and reading the commentary there,
 * avoiding perpetuating the same incomplete and somewhat confusing commentary here.
 *
 * @see platform_frameworks_base/base/core/java/com/android/internal/graphics/palette/WuQuantizer.java
 */
class WuQuantizer : public Quantizer
{
public:
    ~WuQuantizer() override = default;

    void quantize(const std::vector<QRgb> &pixels, int colorCount) override;

private:
    enum Direction {
        Red,
        Green,
        Blue,
    };

    struct MaximizeResult {
        // < 0 if cut impossible
        int m_cutLocation;
        double m_maximum;
    };

    struct CreateBoxesResult {
        int m_requestedCount;
        int m_resultCount;
    };

    struct Box {
        int r0 = 0;
        int r1 = 0;
        int g0 = 0;
        int g1 = 0;
        int b0 = 0;
        int b1 = 0;
        int vol = 0;
    };

    int getIndex(int r, int g, int b) const;
    void constructHistogram(const std::map<QRgb, unsigned> &pixels);
    void createMoments();
    CreateBoxesResult createBoxes(int maxColorCount);
    std::vector<QRgb> createResult(int colorCount) const;
    double variance(const Box &cube) const;
    bool cut(Box &one, Box &two) const;
    MaximizeResult maximize(const Box &cube, Direction direction, int first, int last, int wholeR, int wholeG, int wholeB, int wholeW) const;
    int volume(const Box &cube, const std::array<int, s_totalSize> &moment) const;
    int bottom(const Box &cube, Direction direction, const std::array<int, s_totalSize> &moment) const;
    int top(const Box &cube, Direction direction, int position, const std::array<int, s_totalSize> &moment) const;

    std::array<int, s_totalSize> m_weights;
    decltype(m_weights) m_momentsR;
    decltype(m_weights) m_momentsG;
    decltype(m_weights) m_momentsB;
    std::array<double, s_totalSize> m_moments;
    std::vector<Box> m_cubes;
};

#endif // WUQUANTIZER_H
