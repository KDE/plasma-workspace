/* Wrap XLIB code in a new file as it defines keywords that conflict with Qt

    Copyright (C) 2017 <davidedmundson@kde.org> David Edmundson

    SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "xtestsender.h"
#include <X11/extensions/XTest.h>

void sendXTestPressed(Display *display, int button)
{
    XTestFakeButtonEvent(display, button, true, 0);
}

void sendXTestReleased(Display *display, int button)
{
    XTestFakeButtonEvent(display, button, false, 0);
}
