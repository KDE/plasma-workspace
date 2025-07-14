// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2025 Harald Sitter <sitter@kde.org>

#pragma

#include <QLoggingCategory>
#include <QString>

namespace Levenshtein
{

inline int distance(const QStringView &name, const QStringView &query)
{
    if (name == query) {
        return 0;
    }

    std::vector<int> distance0(query.size() + 1, 0);
    std::vector<int> distance1(query.size() + 1, 0);

    for (int i = 0; i <= query.size(); ++i) {
        distance0[i] = i;
    }

    for (int i = 0; i < name.size(); ++i) {
        distance1[0] = i + 1;
        for (int j = 0; j < query.size(); ++j) {
            const auto deletionCost = distance0[j + 1] + 1;
            const auto insertionCost = distance1[j] + 1;
            const auto substitutionCost = [&] {
                if (name[i] == query[j]) {
                    return distance0[j];
                }
                return distance0[j] + 1;
            }();
            distance1[j + 1] = std::min({deletionCost, insertionCost, substitutionCost});
        }
        std::swap(distance0, distance1);
    }
    return distance0[query.size()];
}

inline qreal score(const QStringView &name, int distance)
{
    // Normalize the distance to a value between 0.0 and 1.0
    // The maximum distance is the length of the pattern.
    // If the distance is 0, it means a perfect match, so we return 1.0.
    // If the distance is equal to the length of the pattern, we return 0.0.
    if (distance == 0) {
        return 1.0;
    }
    if (distance >= name.size()) {
        return 0.0;
    }
    return 1.0 - (qreal(distance) / qreal(name.size()));
}

} // namespace Levenshtein
