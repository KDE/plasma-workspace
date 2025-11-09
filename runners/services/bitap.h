// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma once

#include <bitset>
#include <memory>
#include <optional>

#include <QDebug>
#include <QLoggingCategory>
#include <QString>

namespace Bitap
{

Q_DECLARE_LOGGING_CATEGORY(BITAP)
Q_LOGGING_CATEGORY(BITAP, "org.kde.plasma.runner.services.bitap", QtWarningMsg)

struct Match {
    qsizetype size;
    qsizetype distance;

    bool operator==(const Match &other) const = default;
};

inline QDebug operator<<(QDebug dbg, const Bitap::Match &match)
{
    dbg.nospace() << "Bitap::Match(" << match.size << ", " << match.distance << ")";
    return dbg;
}

// Bitap is a bit of a complicated algorithm thanks to bitwise operations. I've opted to replace them with bitsets for readability.
// It creates a patternMask based on all characters in the pattern. Basically each character gets assigned a representative bit.
// e.g. in the pattern 'abc' the character 'a' would be 110, 'b' 101, 'c' 011.
// This is a bit expensive up front but allows it to carry out everything else using bitwise operations.
// For each match we set a matching bit in the bits vector.
// Matching happens within a hamming distance, meaning up to `hammingDistance` characters can be out of place.
inline std::optional<Match> bitap(const QStringView &name, const QStringView &pattern, int hammingDistance)
{
    qCDebug(BITAP) << "Bitap called with name:" << name << "and pattern:" << pattern << "with hamming distance:" << hammingDistance;
    if (name == pattern) {
        return Match{.size = pattern.size(), .distance = 0}; // Perfect match
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
    if (pattern.size() >= qsizetype(Mask().size()) - 1) {
        qCWarning(BITAP) << "Pattern is too long for bitap algorithm, max length is" << Mask().size() - 1;
        return std::nullopt;
    }

    // With hammingDistance >= pattern.length(), it would always match by changing the entire pattern, so clamp it.
    hammingDistance = std::min(hammingDistance, int(pattern.length() - 1));

    // allocate on heap to prevent stackoverflow on musl libc (patternMask ~512k size, musl stack 128k)
    std::unique_ptr<const PatternMask> patternMask = [&pattern, &name] {
        auto patternMask = std::make_unique<PatternMask>();
        // The following is an optimized version of patternMask.fill(Mask().set()); to set all **necessary** bits to 1.
        for (const auto &qchar : pattern) {
            patternMask->at(qchar.unicode()).set();
        }
        for (const auto &qchar : name) {
            patternMask->at(qchar.unicode()).set();
        }

        for (int i = 0; i < pattern.size(); ++i) {
            const auto char_ = pattern.at(i).unicode();
            patternMask->at(char_).reset(i); // unset the relevant index bits
        }

        if (BITAP().isDebugEnabled()) {
            for (const auto &i : pattern) {
                const auto char_ = i.unicode();
                qCDebug(BITAP) << "Pattern mask for" << char_ << "is" << patternMask->at(char_).to_string();
            }
        }

        return patternMask;
    }();

    Match match{
        .size = 0, // 0 means no match found for convenience
        .distance = name.size(),
    };

    std::vector<Mask> bits((hammingDistance + 1), Mask().set().reset(0));
    std::vector<Mask> transpositions(bits.cbegin(), bits.cend());
    for (int i = 0; i < name.size(); ++i) {
        const auto &char_ = name.at(i);
        auto previousBit = bits[0];
        const auto mask = patternMask->at(char_.unicode());
        bits[0] |= mask;
        bits[0] <<= 1;

        for (int j = 1; j <= hammingDistance; ++j) {
            auto bit = bits[j];
            auto current = (bit | mask) << 1;
            // https://en.wikipedia.org/wiki/Damerau%E2%80%93Levenshtein_distance
            auto substitute = previousBit << 1;
            auto delete_ = bits[j - 1] << 1;
            auto insert = previousBit;
            auto transpose = (transpositions[j - 1] | (mask << 1)) << 1;
            bits[j] = current & substitute & transpose & delete_ & insert;
            transpositions[j - 1] = (previousBit << 1) | mask;
            previousBit = bit;
        }

        if (BITAP().isDebugEnabled()) {
            qCDebug(BITAP) << "After processing character" << char_ << "at index" << i;
            for (const auto &bit : bits) {
                qCDebug(BITAP) << "bit" << bit.to_string();
            }
        }

        for (int k = 0; k <= hammingDistance; ++k) {
            // If the bit at the end of the mask is 0, it means we have a match.
            if (0 == (bits[k] & Mask().set(pattern.size()))) {
                const int newSize = std::min(qsizetype(i + 1), pattern.size());
                if (k < match.distance && match.size <= newSize) {
                    qCDebug(BITAP) << "Match found at index" << i << "with hamming distance" << k << "better than previous match with distance"
                                   << match.distance << "with size" << match.size;

                    match = {
                        .size = newSize,
                        .distance = k,
                    };
                }
                // We do not return early because we want to find the best match, not just any.
                // e.g. with a maximum distance of 1 `disc` could match `disc` either at index two with distance one, or at index three with distance zero.
            }
        }
    }

    // Because we use a complete Damerau–Levenshtein distance the return value is a bit complicated. The trick is that the distance incurs a negative penalty
    // in relation to the max distance. While an end that is closer to the real end is generally favorably. Combining the two into a single value
    // would complicate the meaning of the return value to mean "approximate end with random penalty". This is garbage to reason about so instead we return
    // both values and then assign them meaning in the score function.
    if (match.size > 0) {
        return match;
    }

    qCDebug(BITAP) << "No match found for pattern" << pattern << "in name" << name;
    return std::nullopt;
}

inline qreal score(const QStringView &name, const auto &match, auto hammingDistance)
{
    // Normalize the score to a value between 0.0 and 1.0
    // No distance means the score is directly correlated to the end index. The more characters matched the higher the score.
    // Any distance will lower the score by a sub 0.1 margin.

    if (name.size() == 0) {
        return 0.0; // No name, no score.
    }
    Q_ASSERT(hammingDistance > 0); // The semantics for distance 0 are undefined at this time! If you need this come up with something

    const auto maxSize = name.size();
    const auto penalty = [&] {
        constexpr auto tenth = 10.0;
        constexpr auto half = 2.0;
        return qreal(match.distance) / qreal(hammingDistance) / tenth / half;
    }();
    auto score = qreal(match.size) / qreal(maxSize);
    // Prevent underflows when the penalty is larger than the score.
    score = std::max(0.0, score - penalty);

    Q_ASSERT(score >= 0.0 && score <= 1.0);
    return score;
}

} // namespace Bitap
