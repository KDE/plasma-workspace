/*
    SPDX-FileCopyrightText: 2005 Lubos Lunak <l.lunak@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kcheckrunning.h"
#include <X11/Xlib.h>

/*
 Return 0 when KDE is running, 1 when KDE is not running but it is possible
 to connect to X, 2 when it's not possible to connect to X.
*/
CheckRunningState kCheckRunning()
{
    Display *dpy = XOpenDisplay(nullptr);
    if (dpy == nullptr)
        return NoX11;
    Atom atom = XInternAtom(dpy, "_KDE_RUNNING", False);
    return XGetSelectionOwner(dpy, atom) != None ? PlasmaRunning : NoPlasmaRunning;
}
