// SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#pragma once

#include <array>
#include <cstring>

inline char *safe_strerror(int error)
{
    constexpr auto maxBufferSize = 1024;
    thread_local std::array<char, maxBufferSize> buffer;
    // The return value changes depending on CFLAGS, so we intentionally do not do anything with it!
    strerror_r(error, buffer.data(), buffer.size());
    return buffer.data();
}
