/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

class CursorTheme;
/** Applies a given theme, using XFixes, XCursor and KGlobalSettings.
    @param theme The cursor theme to be applied. It is save to pass 0 here
        (will result in \e false as return value).
    @param size The size hint that is used to select the cursor size.
    @returns If the changes could be applied. Will return \e false if \e theme is
        0 or if the XFixes and XCursor libraries aren't available in the required
        version, otherwise returns \e true. */
bool applyTheme(const CursorTheme *theme, const int size);
