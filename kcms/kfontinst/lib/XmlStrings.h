#pragma once

/*
 * SPDX-FileCopyrightText: 2003-2009 Craig Drummond <craig@kde.org>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QLatin1String>

#define FONTLIST_TAG "fontlist"
inline constexpr QLatin1String FONT_TAG("font");
inline constexpr QLatin1String FILE_TAG("file");
inline constexpr QLatin1String PATH_ATTR("path");
inline constexpr QLatin1String NAME_ATTR("name");
inline constexpr QLatin1String FOUNDRY_ATTR("foundry");
inline constexpr QLatin1String FAMILY_ATTR("family");
inline constexpr QLatin1String FAMILY_TAG("family");
inline constexpr QLatin1String WEIGHT_ATTR("weight");
inline constexpr QLatin1String WIDTH_ATTR("width");
inline constexpr QLatin1String SLANT_ATTR("slant");
inline constexpr QLatin1String SCALABLE_ATTR("scalable");
inline constexpr QLatin1String FACE_ATTR("face");
inline constexpr QLatin1String LANGS_ATTR("langs");
#define SYSTEM_ATTR "system"
inline constexpr QLatin1String ERROR_ATTR("error");
inline constexpr QLatin1Char LANG_SEP(',');
