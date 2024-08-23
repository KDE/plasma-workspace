/*
    SPDX-FileCopyrightText: 2024 Noah Davis <noahadvs@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QString>
#include <algorithm>

namespace Mimetypes
{
using namespace Qt::StringLiterals;

// "MIME type" is the most commonly used name for MIME types on Linux/XDG,
// but they are also called "media types" in other contexts.

/* https://en.wikipedia.org/wiki/Media_type#Structure
A media type consists of a type and a subtype, which is further structured into
a tree. A media type can optionally define a suffix and parameters:

    mime-type = type "/" [tree "."] subtype ["+" suffix]* [";" parameter];

As an example, an HTML file might be designated `text/html; charset=UTF-8`.
In this example, `text` is the type, `html` is the subtype, and `charset=UTF-8`
is an optional parameter indicating the character encoding.

Types, subtypes, and parameter names are case-insensitive. Parameter values are
usually case-sensitive, but may be interpreted in a case-insensitive fashion
depending on the intended use.
*/

namespace Type
{
static const auto image = u"image/"_s;
static const auto text = u"text/"_s;
} // namespace Type

namespace NoType
{
// Hint for klipper to show an item even if non-text copying is disabled.
static const auto xKdeForceImageCopy = u"x-kde-force-image-copy"_s;
// Hint for klipper to hide an item if it has the value "secret".
static const auto xKdePasswordManagerHint = u"x-kde-passwordManagerHint"_s;
} // namespace NoType

namespace Application
{
// Basically a QImage.
// Set when QMimeData::setImageData is used.
static const auto xQtImage = u"application/x-qt-image"_s;
// A color, usually from a color selection widget.
// Usually dragged and dropped, but technically copyable to the clipboard.
static const auto xColor = u"application/x-color"_s;
// Klipper hint to not empty the clipboard.
static const auto xKdeOnlyReplaceEmpty = u"application/x-kde-onlyReplaceEmpty"_s;
// When plasmashell is not focused, klipper will not receive new clip data
// immediately. This type is used to filter out selections.
static const auto xKdeSyncselection = u"application/x-kde-syncselection"_s;
// KIO hint that urls are involved in a cut operation.
static const auto xKdeCutselection = u"application/x-kde-cutselection"_s;
} // namespace Application

namespace Image
{
static const auto bmp = u"image/bmp"_s;
static const auto png = u"image/png"_s;
} // namespace Image

namespace Text
{
// Plain text data.
static const auto plain = u"text/plain"_s;
// UTF-8 plain text data
static const auto plainUtf8 = u"text/plain;charset=utf-8"_s;
// HTML text data
static const auto html = u"text/html"_s;
// Markdown text data
static const auto markdown = u"text/markdown"_s;
// URL/URI list.
static const auto uriList = u"text/uri-list"_s;
} // namespace Text

// Utility functions
namespace Utils
{
inline bool hasType(QAnyStringView mimetype, QAnyStringView type)
{
    return mimetype.size() >= type.size() && mimetype.first(type.size()) == type;
}

template<typename Container, typename Predicate>
inline bool anyOf(const Container &container, Predicate predicate)
{
    return std::any_of(container.begin(), container.end(), predicate);
}

template<typename Container>
inline bool anyOfType(const Container &mimetypes, QAnyStringView type)
{
    return anyOf(mimetypes, [type](QAnyStringView mimetype) {
        return hasType(mimetype, type);
    });
}

inline bool isPlainText(QAnyStringView mimetype)
{
    return mimetype == Text::plain || mimetype == Text::plainUtf8;
}

inline bool isImage(QAnyStringView mimetype)
{
    return hasType(mimetype, Type::image) || mimetype == Application::xQtImage;
}
} // namespace Utils
} // namespace Mimetype
