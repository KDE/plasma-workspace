/*
    SPDX-FileCopyrightText: 2021 Ahmad Samir <a.samirh78@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QDBusConnection>
#include <QDBusMessage>

// These two enums are copied from KHintSettings (which copied them from KGlobalSettings)
enum GlobalChangeType {
    PaletteChanged = 0,
    FontChanged,
    StyleChanged, // 2
    SettingsChanged,
    IconChanged,
    CursorChanged, // 5
    ToolbarStyleChanged,
    ClipboardConfigChanged,
    BlockShortcuts,
    NaturalSortingChanged,
};

enum GlobalSettingsCategory {
    SETTINGS_MOUSE,
    SETTINGS_COMPLETION,
    SETTINGS_PATHS,
    SETTINGS_POPUPMENU,
    SETTINGS_QT,
    SETTINGS_SHORTCUTS,
    SETTINGS_LOCALE,
    SETTINGS_STYLE,
};

void notifyKcmChange(GlobalChangeType changeType, int arg = 0);
