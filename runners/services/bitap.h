// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <bitset>
#include <optional>

#include <QDebug>
#include <QLoggingCategory>
#include <QString>

namespace Bitap
{

Q_DECLARE_LOGGING_CATEGORY(BITAP)
Q_LOGGING_CATEGORY(BITAP, "org.kde.plasma.runner.services.bitap", QtWarningMsg)

// Bitap is a bit of a complicated algorithm thanks to bitwise operations. I've opted to replace them with bitsets for readability.
// It creates a patternMask based on all characters in the pattern. Basically each character gets assigned a representative bit.
// e.g. in the pattern 'abc' the character 'a' would be 110, 'b' 101, 'c' 011.
// This is a bit expensive up front but allows it to carry out everything else using bitwise operations.
// For each match we set a matching bit in the bits vector.
// Matching happens within a hamming distance, meaning up to `hammingDistance` characters can be out of place.
inline std::optional<qsizetype> bitap(const QStringView &name, const QStringView &pattern, int hammingDistance)
{
    qCDebug(BITAP) << "Bitap called with name:" << name << "and pattern:" << pattern << "with hamming distance:" << hammingDistance;
    if (name == pattern) {
        return 0; // Perfect match, return the index.
    }

    if (pattern.isEmpty() || name.isEmpty()) {
        return std::nullopt;
    }

    // Being a bitset we could have any number of bits, but practically we probably don't need more than 64, most bitaps I've seen even use 32.
    constexpr auto maxMaskBits = 64;
    using Mask = std::bitset<maxMaskBits>;
    using PatternMask = std::array<Mask, std::numeric_limits<char16_t>::max()>;

    // The way bitap works is that each bit of the Mask represents a character position. Because of this we cannot match
    // more characters than we have bits for.
    // -1 because one bit is used for the result (I think)
    if (pattern.size() >= Mask().size() - 1) {
        qCWarning(BITAP) << "Pattern is too long for bitap algorithm, max length is" << Mask().size() - 1;
        return std::nullopt;
    }

    const PatternMask patternMask = [&pattern] {
        thread_local PatternMask patternMask;
        patternMask.fill(Mask().set()); // set all bits to 1

        for (int i = 0; i < pattern.size(); ++i) {
            const auto char_ = pattern.at(i).unicode();
            patternMask.at(char_).reset(i); // unset the relevant index bits
        }

        if (BITAP().isDebugEnabled()) {
            for (const auto &i : pattern) {
                const auto char_ = i.unicode();
                qCDebug(BITAP) << "Pattern mask for" << char_ << "is" << patternMask.at(char_).to_string();
            }
        }

        return patternMask;
    }();

    std::vector<Mask> bits((hammingDistance + 1), Mask().set().reset(0));
    for (int i = 0; i < name.size(); ++i) {
        const auto &char_ = name.at(i);
        auto previousBit = bits[0];
        bits[0] |= patternMask.at(char_.unicode());
        bits[0] <<= 1;

        for (auto &bit : std::span<Mask>(bits).subspan(1)) {
            // Hamming distance happens here, for substitions exclusively (bit | char).
            previousBit = (previousBit & (bit | patternMask.at(char_.unicode()))) << 1;
            std::swap(bit, previousBit);
        }

        if (BITAP().isDebugEnabled()) {
            for (const auto &bit : bits) {
                qCDebug(BITAP) << "bit" << bit.to_string();
            }
        }

        if (0 == (bits[hammingDistance] & Mask().set(pattern.size()))) {
            qCDebug(BITAP) << "Match found at index" << i << Mask().set(pattern.size()).to_string();
            return (i - pattern.size()) + 1; // Return the index of the match
        }
    }

    qCDebug(BITAP) << "No match found for pattern" << pattern << "in name" << name;
    return std::nullopt;
}

inline qreal score(const auto &name, qsizetype distance)
{
    // Normalize the distance to a value between 0.0 and 1.0
    // The maximum distance is the length of the pattern.
    // If the distance is 0, it means a perfect match, so we return 1.0.
    // If the distance is equal to the length of the pattern, we return 0.0.
    if (distance == 0) {
        return 1.0;
    }
    return 1.0 - (static_cast<qreal>(distance) / name.size());
}

} // namespace Bitap
