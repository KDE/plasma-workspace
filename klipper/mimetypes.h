// SPDX-FileCopyrightText: 2026 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include <QLatin1StringView>

namespace MimeTypes
{
using namespace Qt::StringLiterals;

// Categories
inline constexpr auto imagePrefix = "image/"_L1;
inline constexpr auto textPrefix = "text/"_L1;

// Image Formats
inline constexpr auto png = "image/png"_L1;
inline constexpr auto jpeg = "image/jpeg"_L1;
inline constexpr auto qtImage = "application/x-qt-image"_L1;

// Text Formats
inline constexpr auto uriList = "text/uri-list"_L1;
inline constexpr auto plainText = "text/plain"_L1;
inline constexpr auto plainUtf8Text = "text/plain;charset=utf-8"_L1;
inline constexpr auto html = "text/html"_L1;
inline constexpr auto json = "application/json"_L1;
inline constexpr auto xml = "application/xml"_L1;

// Hints/Metadata Formats
inline constexpr auto kdeSuggestedFileName = "application/x-kde-suggestedfilename"_L1;
inline constexpr auto kdeForceImageCopy = "x-kde-force-image-copy"_L1;
}
