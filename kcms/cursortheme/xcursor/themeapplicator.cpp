/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "config-X11.h"

#include "cursortheme.h"

#include "../../krdb/krdb.h"

#include <QFile>
#include <private/qtx11extras_p.h>

#include <X11/Xcursor/Xcursor.h>
#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

using namespace Qt::StringLiterals;

bool applyTheme(const CursorTheme *theme, const int size)
{
    // Require the Xcursor version that shipped with X11R6.9 or greater, since
    // in previous versions the Xfixes code wasn't enabled due to a bug in the
    // build system (freedesktop bug #975).
#if HAVE_XFIXES && XFIXES_MAJOR >= 2 && XCURSOR_LIB_VERSION >= 10105
    if (!theme) {
        return false;
    }

    // Update the Xcursor X resources
    runRdb(0);

    // Reload the standard cursors
    QStringList names;

    if (CursorTheme::haveXfixes()) {
        // Qt cursors
        // clang-format off
        names << u"left_ptr"_s
              << u"up_arrow"_s
              << u"cross"_s
              << u"wait"_s
              << u"left_ptr_watch"_s
              << u"ibeam"_s
              << u"size_ver"_s
              << u"size_hor"_s
              << u"size_bdiag"_s
              << u"size_fdiag"_s
              << u"size_all"_s
              << u"split_v"_s
              << u"split_h"_s
              << u"pointing_hand"_s
              << u"openhand"_s
              << u"closedhand"_s
              << u"forbidden"_s
              << u"whats_this"_s
              << u"copy"_s
              << u"move"_s
              << u"link"_s;

        // X core cursors
        names << u"X_cursor"_s
              << u"right_ptr"_s
              << u"hand1"_s
              << u"hand2"_s
              << u"watch"_s
              << u"xterm"_s
              << u"crosshair"_s
              << u"left_ptr_watch"_s
              << u"center_ptr"_s
              << u"sb_h_double_arrow"_s
              << u"sb_v_double_arrow"_s
              << u"fleur"_s
              << u"top_left_corner"_s
              << u"top_side"_s
              << u"top_right_corner"_s
              << u"right_side"_s
              << u"bottom_right_corner"_s
              << u"bottom_side"_s
              << u"bottom_left_corner"_s
              << u"left_side"_s
              << u"question_arrow"_s
              << u"pirate"_s;
        // clang-format on

        for (const QString &name : std::as_const(names)) {
            XFixesChangeCursorByName(QX11Info::display(), theme->loadCursor(name, size), QFile::encodeName(name).constData());
        }
    }

    return true;
#else
    Q_UNUSED(theme)
    Q_UNUSED(size)
    return false;
#endif
}
