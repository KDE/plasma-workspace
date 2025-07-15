// Port of fzf's algo.go to Qt
// https://github.com/junegunn/fzf/blob/84e515bd6ecff0ed07d1373ff8e770f91d933b2e/src/algo/algo.go
// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2013-2025 Junegunn Choi
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

/*
Algorithm
---------
FuzzyMatchV1 finds the first "fuzzy" occurrence of the pattern within the given
text in O(n) time where n is the length of the text. Once the position of the
last character is located, it traverses backwards to see if there's a shorter
substring that matches the pattern.

    a_____b___abc__  To find "abc"
    *-----*-----*>   1. Forward scan
             <***    2. Backward scan

The algorithm is simple and fast, but as it only sees the first occurrence,
it is not guaranteed to find the occurrence with the highest score.

    a_____b__c__abc
    *-----*--*  ***

FuzzyMatchV2 implements a modified version of Smith-Waterman algorithm to find
the optimal solution (highest score) according to the scoring criteria. Unlike
the original algorithm, omission or mismatch of a character in the pattern is
not allowed.

Performance
-----------
The new V2 algorithm is slower than V1 as it examines all occurrences of the
pattern instead of stopping immediately after finding the first one. The time
complexity of the algorithm is O(nm) if a match is found and O(n) otherwise
where n is the length of the item and m is the length of the pattern. Thus, the
performance overhead may not be noticeable for a query with high selectivity.
However, if the performance is more important than the quality of the result,
you can still choose v1 algorithm with --algo=v1.

Scoring criteria
----------------

- We prefer matches at special positions, such as the start of a word, or
  uppercase character in camelCase words.

- That is, we prefer an occurrence of the pattern with more characters
  matching at special positions, even if the total match length is longer.
    e.g. "fuzzyfinder" vs. "fuzzy-finder" on "ff"
                            ````````````
- Also, if the first character in the pattern appears at one of the special
  positions, the bonus point for the position is multiplied by a constant
  as it is extremely likely that the first character in the typed pattern
  has more significance than the rest.
    e.g. "fo-bar" vs. "foob-r" on "br"
          ``````
- But since fzf is still a fuzzy finder, not an acronym finder, we should also
  consider the total length of the matched substring. This is why we have the
  gap penalty. The gap penalty increases as the length of the gap (distance
  between the matching characters) increases, so the effect of the bonus is
  eventually cancelled at some point.
    e.g. "fuzzyfinder" vs. "fuzzy-blurry-finder" on "ff"
          ```````````
- Consequently, it is crucial to find the right balance between the bonus
  and the gap penalty. The parameters were chosen that the bonus is cancelled
  when the gap size increases beyond 8 characters.

- The bonus mechanism can have the undesirable side effect where consecutive
  matches are ranked lower than the ones with gaps.
    e.g. "foobar" vs. "foo-bar" on "foob"
                       ```````
- To correct this anomaly, we also give extra bonus point to each character
  in a consecutive matching chunk.
    e.g. "foobar" vs. "foo-bar" on "foob"
          ``````
- The amount of consecutive bonus is primarily determined by the bonus of the
  first character in the chunk.
    e.g. "foobar" vs. "out-of-bound" on "oob"
                       ````````````
*/

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <QDebug>
#include <QString>

using namespace Qt::StringLiterals;

std::string delimiterChars = "/,:;|";

struct Result {
    int Start;
    int End;
    int Score;
};

constexpr int scoreMatch = 16;
constexpr int scoreGapStart = -3;
constexpr int scoreGapExtension = -1;
constexpr int bonusBoundary = scoreMatch / 2;
constexpr int bonusNonWord = scoreMatch / 2;
constexpr int bonusCamel123 = bonusBoundary + scoreGapExtension;
constexpr int16_t bonusConsecutive = -(scoreGapStart + scoreGapExtension);
constexpr int bonusFirstCharMultiplier = 2;

int16_t bonusBoundaryWhite = bonusBoundary + 2;
int16_t bonusBoundaryDelimiter = bonusBoundary + 1;

enum charClass {
    charWhite,
    charNonWord,
    charDelimiter,
    charLower,
    charUpper,
    charLetter,
    charNumber
};

charClass initialCharClass = charWhite;
std::array<charClass, std::numeric_limits<char>::max()> asciiCharClasses;
int16_t bonusMatrix[charNumber + 1][charNumber + 1];

int16_t bonusFor(charClass prevClass, charClass class_);
charClass charClassOfNonAscii(const QChar &ch);

namespace util
{
struct normalizer {
    QChar operator[](QChar r) const
    {
        // FIXME very hacky!
        auto decomposition = r.decomposition();
        return decomposition.isEmpty() ? r : r.decomposition().at(0);
    }
} normalized;
}

bool Init(const QLatin1StringView &scheme)
{
    if (scheme == "default"_L1) {
        bonusBoundaryWhite = bonusBoundary + 2;
        bonusBoundaryDelimiter = bonusBoundary + 1;
    } else if (scheme == "path"_L1) {
        bonusBoundaryWhite = bonusBoundary;
        bonusBoundaryDelimiter = bonusBoundary + 1;
        delimiterChars = "/";
        initialCharClass = charDelimiter;
    } else if (scheme == "history"_L1) {
        bonusBoundaryWhite = bonusBoundary;
        bonusBoundaryDelimiter = bonusBoundary;
    } else {
        return false;
    }
    for (size_t i = 0; i < asciiCharClasses.size(); i++) {
        asciiCharClasses[i] = charClassOfNonAscii(QLatin1Char(i));
    }
    for (int i = 0; i <= charNumber; i++) {
        for (int j = 0; j <= charNumber; j++) {
            bonusMatrix[i][j] = bonusFor(static_cast<charClass>(i), static_cast<charClass>(j));
        }
    }
    return true;
}

std::vector<int> *posArray(bool withPos, int len)
{
    if (withPos) {
        return new std::vector<int>();
    }
    return nullptr;
}

charClass charClassOfNonAscii(const QChar &ch)
{
    if (ch.isLower())
        return charLower;
    else if (ch.isUpper())
        return charUpper;
    else if (ch.isDigit())
        return charNumber;
    else if (ch.isLetter())
        return charLetter;
    else if (ch.isSpace())
        return charWhite;
    else if (delimiterChars.find(ch.unicode()) != std::string::npos)
        return charDelimiter;
    return charNonWord;
}

charClass charClassOf(const QChar &ch)
{
    if (auto unicode = ch.unicode(); unicode < asciiCharClasses.size())
        return asciiCharClasses[unicode];
    return charClassOfNonAscii(ch);
}

int16_t bonusFor(charClass prevClass, charClass class_)
{
    if (class_ > charNonWord) {
        switch (prevClass) {
        case charWhite:
            return bonusBoundaryWhite;
        case charDelimiter:
            return bonusBoundaryDelimiter;
        case charNonWord:
            return bonusBoundary;
        }
    }
    if ((prevClass == charLower && class_ == charUpper) || (prevClass != charNumber && class_ == charNumber)) {
        return bonusCamel123;
    }
    switch (class_) {
    case charNonWord:
    case charDelimiter:
        return bonusNonWord;
    case charWhite:
        return bonusBoundaryWhite;
    }
    return 0;
}

int16_t bonusAt(const QString &input, int idx)
{
    if (idx == 0)
        return bonusBoundaryWhite;
    return bonusMatrix[charClassOf(input.at(idx - 1))][charClassOf(input.at(idx))];
}

QChar normalizeRune(const QChar &r)
{
    const auto unicode = r.unicode();
    if (unicode < 0x00C0 || unicode > 0xFF61)
        return r;
    auto normalized = util::normalized[r];
    if (normalized.isNull())
        return r;
    return normalized;
}

bool isAscii(const QString &runes)
{
    for (auto r : runes) {
        if (r.toLatin1() <= 0) {
            return false;
        }
    }
    return true;
}

int trySkip(const QStringView &input, bool caseSensitive, const QChar &b, int from)
{
    auto byteArray = input.mid(from);
    auto idx = byteArray.indexOf(b);
    if (idx == 0) {
        // Can't skip any further
        return from;
    }
    // We may need to search for the uppercase letter again. We don't have to
    // consider normalization as we can be sure that this is an ASCII string.
    if (!caseSensitive && b >= 'a'_L1 && b <= 'z'_L1) {
        if (idx > 0)
            byteArray = byteArray.mid(0, idx);
        auto uidx = byteArray.indexOf(QChar(b.unicode() - 32));
        if (uidx >= 0)
            idx = uidx;
    }
    if (idx < 0)
        return -1;
    return from + idx;
}

std::pair<int, int> asciiFuzzyIndex(const QString &input, const QString &pattern, bool caseSensitive)
{
    // FIXME what is this about
    // // Can't determine
    // if (!input->IsBytes()) return {0, input->Length()};

    if (!isAscii(pattern))
        return {-1, -1};

    qsizetype firstIdx = 0;
    qsizetype idx = 0;
    qsizetype lastIdx = 0;
    QChar b;
    for (qsizetype pidx = 0; pidx < pattern.size(); pidx++) {
        b = pattern[pidx];
        idx = trySkip(input, caseSensitive, b, idx);
        if (idx < 0)
            return {-1, -1};

        if (pidx == 0 && idx > 0)
            firstIdx = idx - 1;

        lastIdx = idx;
        idx++;
    }

    // Find the last appearance of the last character of the pattern to limit the search scope
    QChar bu = b;
    // TODO: what is this about?
    if (!caseSensitive && b >= 'a'_L1 && b <= 'z'_L1)
        bu = QChar(b.unicode() - 32);
    auto scope = input.mid(lastIdx);
    for (int offset = scope.size() - 1; offset > 0; offset--) {
        if (scope[offset] == b || scope[offset] == bu) {
            return {firstIdx, lastIdx + offset + 1};
        }
    }
    return {firstIdx, lastIdx + 1};
}

void debugV2(const auto &T, const auto &pattern, const auto &F, int lastIdx, const auto &H, const auto &C)
{
    int width = lastIdx - F[0] + 1;
    for (size_t i = 0; i < F.size(); i++) {
        auto f = F[i];
        int I = i * width;
        if (i == 0) {
            printf("  ");
            for (int j = f; j <= lastIdx; j++) {
                printf(" %c ", T[j]);
            }
            printf("\n");
        }
        printf("%c ", pattern[i]);
        for (int idx = F[0]; idx < f; idx++) {
            printf(" 0 ");
        }
        for (int idx = f; idx <= lastIdx; idx++) {
            printf("%2d ", H[i * width + idx - F[0]]);
        }

        printf("\n  ");
        auto cSub = std::span(C.begin() + I, C.begin() + I + width);
        for (int idx = 0; idx < cSub.size(); idx++) {
            auto p = C[idx];
            if (idx + F[0] < F[i]) {
                p = 0;
            }
            if (p > 0) {
                printf("%2d ", C[I + idx]);
            } else {
                printf("   ");
            }
        }
        printf("\n");
    }
}

std::pair<Result, std::vector<int> *> FuzzyMatchV2(bool caseSensitive, bool normalize, bool forward, const QString &input, const QString &pattern, bool withPos)
{
    // Assume that pattern is given in lowercase if case-insensitive.
    // First check if there's a match and calculate bonus for each position.
    // If the input string is too long, consider finding the matching chars in
    // this phase as well (non-optimal alignment).
    int M = pattern.size();
    if (M == 0)
        return {Result{0, 0, 0}, posArray(withPos, M)};
    int N = input.size();
    if (M > N)
        return {Result{-1, -1, 0}, nullptr};

    // TODO
    // // Since O(nm) algorithm can be prohibitively expensive for large input,
    // // we fall back to the greedy algorithm.
    // if (slab && N * M > slab->I16.capacity()) {
    //     return FuzzyMatchV1(caseSensitive, normalize, forward, input, pattern, withPos, slab);
    // }

    // Phase 1. Optimized search for ASCII string
    auto idxs = asciiFuzzyIndex(input, pattern, caseSensitive);
    auto minIdx = idxs.first;
    auto maxIdx = idxs.second;
    if (minIdx < 0) {
        return {Result{-1, -1, 0}, nullptr};
    }
    // fmt.Println(N, maxIdx, idx, maxIdx-idx, input.ToString())
    N = maxIdx - minIdx;

    // Reuse pre-allocated integer slice to avoid unnecessary sweeping of garbages
    std::vector<int16_t> H0(N, 0);
    std::vector<int16_t> C0(N, 0);
    // Bonus point for each position
    std::vector<int16_t> B(N, 0);
    // The first occurrence of each character in the pattern
    std::vector<int32_t> F(M, 0);
    QString T = input.mid(minIdx, N);

    // Phase 2. Calculate bonus for each point
    int16_t maxScore = 0;
    int maxScorePos = 0;
    int pidx = 0;
    int lastIdx = 0;
    QChar pchar0 = pattern[0];
    QChar pchar = pattern[0];
    int16_t prevH0 = 0;
    charClass prevClass = initialCharClass;
    bool inGap = false;
    for (qsizetype off = 0; off < T.size(); off++) {
        QChar char_ = T[off];
        charClass class_;
        if (char_.unicode() < asciiCharClasses.size()) {
            class_ = asciiCharClasses[char_.unicode()];
            if (!caseSensitive && class_ == charUpper) {
                char_ = QChar(char_.unicode() + 32); // FIXME WHYYYYYYYYYY?
                T[off] = char_;
            }
        } else {
            class_ = charClassOfNonAscii(char_);
            if (!caseSensitive && class_ == charUpper) {
                char_ = char_.toLower();
            }
            if (normalize)
                char_ = normalizeRune(char_);
            T[off] = char_;
        }
        int16_t bonus = bonusMatrix[prevClass][class_];
        B[off] = bonus;
        prevClass = class_;
        if (char_ == pchar) {
            if (pidx < M) {
                F[pidx] = off;
                pidx++;
                pchar = pattern[std::min(pidx, M - 1)];
            }
            lastIdx = off;
        }
        if (char_ == pchar0) {
            int16_t score = scoreMatch + bonus * bonusFirstCharMultiplier;
            H0[off] = score;
            C0[off] = 1;
            if (M == 1 && ((forward && score > maxScore) || (!forward && score >= maxScore))) {
                maxScore = score;
                maxScorePos = off;
                if (forward && bonus >= bonusBoundary)
                    break;
            }
            inGap = false;
        } else {
            if (inGap)
                H0[off] = std::max(prevH0 + scoreGapExtension, 0);
            else
                H0[off] = std::max(prevH0 + scoreGapStart, 0);
            C0[off] = 0;
            inGap = true;
        }
        prevH0 = H0[off];
    }

    if (pidx != M)
        return {Result{-1, -1, 0}, nullptr};

    if (M == 1) {
        Result result{minIdx + maxScorePos, minIdx + maxScorePos + 1, maxScore};
        if (!withPos)
            return {result, nullptr};
        auto pos = new std::vector<int>{minIdx + maxScorePos};
        return {result, pos};
    }

    // Phase 3. Fill in score matrix (H)
    // Unlike the original algorithm, we do not allow omission.
    int f0 = F[0];
    int width = lastIdx - f0 + 1;
    std::vector<int16_t> H(width * M, 0);
    std::copy(H0.begin() + f0, H0.begin() + lastIdx + 1, H.begin());

    // Possible length of consecutive chunk at each position.
    std::vector<int16_t> C(width * M, 0);
    std::copy(C0.begin() + f0, C0.begin() + lastIdx + 1, C.begin());

    std::vector<int32_t> Fsub(F.begin() + 1, F.end());
    std::vector<QChar> Psub(pattern.begin() + 1, pattern.begin() + 1 + Fsub.size());
    for (size_t off = 0; off < Fsub.size(); off++) {
        int f = Fsub[off];
        const QChar pchar = Psub[off];
        int pidx = off + 1;
        int row = pidx * width;
        bool inGap = false;
        // FIXME aaaaaaaaaaaaaaaaaaaaaaaaaaahhhhhhhhhhhhhhhhhh
        const auto Tsub = std::span(T.begin() + f, T.begin() + lastIdx + 1);
        const auto Bsub = std::span(B.begin() + f, B.begin() + f + Tsub.size());
        const auto Csub = std::span(C.begin() + row + f - f0, C.begin() + row + f - f0 + Tsub.size());
        const auto Cdiag = std::span(C.begin() + row + f - f0 - 1 - width, C.begin() + row + f - f0 - 1 - width + Tsub.size());
        const auto Hsub = std::span(H.begin() + row + f - f0, H.begin() + row + f - f0 + Tsub.size());
        const auto Hdiag = std::span(H.begin() + row + f - f0 - 1 - width, H.begin() + row + f - f0 - 1 - width + Tsub.size());
        const auto Hleft = std::span(H.begin() + row + f - f0 - 1, H.begin() + row + f - f0 - 1 + Tsub.size());
        Hleft[0] = 0;
        for (size_t off2 = 0; off2 < Tsub.size(); off2++) {
            int col = off2 + f;
            int16_t s1 = 0;
            int16_t s2 = 0;
            int16_t consecutive = 0;
            if (inGap) {
                s2 = Hleft[off2] + scoreGapExtension;
            } else {
                s2 = Hleft[off2] + scoreGapStart;
            }

            if (pchar == Tsub[off2]) {
                s1 = Hdiag[off2] + scoreMatch;
                int16_t b = Bsub[off2];
                consecutive = Cdiag[off2] + 1;
                if (consecutive > 1) {
                    int16_t fb = B[col - consecutive + 1];

                    // Break consecutive chunk
                    if (b >= bonusBoundary && b > fb) {
                        consecutive = 1;
                    } else {
                        b = std::max(b, std::max(bonusConsecutive, fb));
                    }
                }
                if (s1 + b < s2) {
                    s1 += Bsub[off2];
                    consecutive = 0;
                } else {
                    s1 += b;
                }
            }
            Csub[off2] = consecutive;

            inGap = s1 < s2;
            auto score = std::max<int16_t>(std::max(s1, s2), 0);
            if (pidx == M - 1 && ((forward && score > maxScore) || (!forward && score >= maxScore))) {
                maxScore = score;
                maxScorePos = col;
            }
            Hsub[off2] = score;
        }
    }

    debugV2(T, pattern, F, lastIdx, H, C);

    // Phase 4. (Optional) Backtrace to find character positions
    auto pos = posArray(withPos, M);
    int j = f0;
    if (withPos) {
        int i = M - 1;
        j = maxScorePos;
        bool preferMatch = true;
        while (true) {
            int I = i * width;
            int j0 = j - f0;
            int16_t s = H[I + j0];

            int16_t s1 = 0;
            int16_t s2 = 0;
            if (i > 0 && j >= F[i]) {
                s1 = H[I - width + j0 - 1];
            }
            if (j > F[i]) {
                s2 = H[I + j0 - 1];
            }

            if (s > s1 && (s > s2 || (s == s2 && preferMatch))) {
                pos->push_back(j + minIdx);
                if (i == 0) {
                    break;
                }
                i--;
            }
            preferMatch = C[I + j0] > 1 || (I + width + j0 + 1 < C.size() && C[I + width + j0 + 1] > 0);
            j--;
        }
    }
    return {Result{minIdx + j, minIdx + maxScorePos + 1, maxScore}, pos};
}
