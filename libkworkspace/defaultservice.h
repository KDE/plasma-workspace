// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#pragma once

#include "kworkspace_export.h"

#include <KService>

namespace DefaultService
{
/// @returns the default browser service (may be invalid if resolution fails)
[[nodiscard]] KWORKSPACE_EXPORT KService::Ptr browser();
/// @returns the **legacy** browser executable only. Use this only as fallback to ::browser()!
[[nodiscard]] KWORKSPACE_EXPORT QString legacyBrowserExec();
} // namespace DefaultService
