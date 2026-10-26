/*
    SPDX-FileCopyrightText: 2022 Popov Eugene <popov895@ukr.net>

    SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>

#include "klipper_export.h"

class KLIPPER_EXPORT Utils
{
public:
    /**
     * Returns a simplified text of a maximum length of maxLength
     */
    static QString simplifiedText(const QString &text, int maxLength);

    /**
     * Find all of the given values in the range.
     * Returns an std::array of iterators from the range.
     */
    template<typename Range, typename... Values, typename Iterators = std::array<std::ranges::iterator_t<Range>, sizeof...(Values)>>
    static constexpr Iterators find(Range &range, Values &&...values)
    {
        const auto begin = std::ranges::begin(range);
        const auto end = std::ranges::end(range);
        Iterators iterators; // Iterators from the range for each value.
        iterators.fill(end); // The value for elements not found.
        // For each range element, find a match.
        for (auto it = begin; it != end && std::ranges::contains(iterators, end); ++it) {
            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                // If not already found and has a match, store the iterator.
                // Parameter pack expansion needs to be done in one expression.
                ((std::get<Is>(iterators) == end && *it == values //
                      ? (std::get<Is>(iterators) = it) // store it
                      : end), // unused
                 ...); // continue to next match attempt
            }(std::index_sequence_for<Values...>{});
        }
        return iterators;
    }
};
