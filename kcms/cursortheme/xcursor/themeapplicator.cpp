/*
    SPDX-FileCopyrightText: 2021 Dan Leinir Turthra Jensen <admin@leinir.dk>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "config-X11.h"

#include "cursortheme.h"

#include "../../krdb/krdb.h"

#include <updatelaunchenvjob.h>

#include <QFile>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <private/qtx11extras_p.h>
#else
#include <QX11Info>
#endif

#include <X11/Xcursor/Xcursor.h>
#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif

bool applyTheme(const CursorTheme *theme, const int size)
{
    // Require the Xcursor version that shipped with X11R6.9 or greater, since
    // in previous versions the Xfixes code wasn't enabled due to a bug in the
    // build system (freedesktop bug #975).
#if HAVE_XFIXES && XFIXES_MAJOR >= 2 && XCURSOR_LIB_VERSION >= 10105
    if (!theme) {
        return false;
    }

    QByteArray themeName = QFile::encodeName(theme->name());

    // Set up the proper launch environment for newly started apps
    UpdateLaunchEnvJob launchEnvJob(QStringLiteral("XCURSOR_THEME"), themeName);

    // Update the Xcursor X resources
    runRdb(0);

    // Reload the standard cursors
    QStringList names;

    if (CursorTheme::haveXfixes()) {
        // Qt cursors
        names << "left_ptr"
              << "up_arrow"
              << "cross"
              << "wait"
              << "left_ptr_watch"
              << "ibeam"
              << "size_ver"
              << "size_hor"
              << "size_bdiag"
              << "size_fdiag"
              << "size_all"
              << "split_v"
              << "split_h"
              << "pointing_hand"
              << "openhand"
              << "closedhand"
              << "forbidden"
              << "whats_this"
              << "copy"
              << "move"
              << "link";

        // X core cursors
        names << "X_cursor"
              << "right_ptr"
              << "hand1"
              << "hand2"
              << "watch"
              << "xterm"
              << "crosshair"
              << "left_ptr_watch"
              << "center_ptr"
              << "sb_h_double_arrow"
              << "sb_v_double_arrow"
              << "fleur"
              << "top_left_corner"
              << "top_side"
              << "top_right_corner"
              << "right_side"
              << "bottom_right_corner"
              << "bottom_side"
              << "bottom_left_corner"
              << "left_side"
              << "question_arrow"
              << "pirate";

        foreach (const QString &name, names) {
            XFixesChangeCursorByName(QX11Info::display(), theme->loadCursor(name, size), QFile::encodeName(name));
        }
    }

    return true;
#else
    Q_UNUSED(theme)
    Q_UNUSED(size)
    return false;
#endif
}
