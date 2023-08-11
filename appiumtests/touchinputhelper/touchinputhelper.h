/*
    SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "touchinputhelper_export.h"

extern "C" {
TOUCHINPUTHELPER_EXPORT void init_application();
TOUCHINPUTHELPER_EXPORT void unload_application();

TOUCHINPUTHELPER_EXPORT void init_task_manager();
TOUCHINPUTHELPER_EXPORT int get_task_count();
/**
 * @return int[4] the geometry of the window at index @p row
 */
TOUCHINPUTHELPER_EXPORT int *get_window_rect(int row);
TOUCHINPUTHELPER_EXPORT int *get_screen_rect(int row);
TOUCHINPUTHELPER_EXPORT void maximize_window(int row);

/**
 * @note A touch_down/move/up event must follow a touch_frame event
 * @see https://wayland.app/protocols/kde-fake-input
 */
TOUCHINPUTHELPER_EXPORT void init_fake_input();
TOUCHINPUTHELPER_EXPORT void touch_down(int x, int y);
TOUCHINPUTHELPER_EXPORT void touch_move(int x, int y);
TOUCHINPUTHELPER_EXPORT void touch_up();
}
