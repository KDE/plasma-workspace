/*
    SPDX-FileCopyrightText: 2021 The Android Open Source Project
    SPDX-FileCopyrightText: 2022 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: Apache-2.0
*/

#include "quantizer.h"

const std::vector<QRgb> &Quantizer::getQuantizedColors() const
{
    return m_palette;
}

const std::map<QRgb, unsigned> &Quantizer::inputPixelToCount() const
{
    return m_inputPixelToCount;
}

void QuantizerMap::quantize(const std::vector<QRgb> &pixels, int)
{
    m_inputPixelToCount.clear();

    for (const QRgb pixel : pixels) {
        auto it = m_inputPixelToCount.find(pixel);
        if (it == m_inputPixelToCount.end()) {
            m_inputPixelToCount.emplace(pixel, 1);
        } else {
            it->second++;
        }
    }

    std::vector<std::pair<QRgb, int>> exportList;
    std::copy(m_inputPixelToCount.cbegin(), m_inputPixelToCount.cend(), std::back_inserter(exportList));
    std::sort(exportList.begin(), exportList.end(), [](const std::pair<QRgb, int> &a, decltype(a) b) {
        return a.second > b.second;
    });
    m_palette.clear();
    m_palette.reserve(exportList.size());
    for (const auto &pr : std::as_const(exportList)) {
        m_palette.emplace_back(pr.first);
    }
}
