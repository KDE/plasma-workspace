/* This file is part of the KDE project
   Copyright (C) 2005 Lubos Lunak <l.lunak@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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
