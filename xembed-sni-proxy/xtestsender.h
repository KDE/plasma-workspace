/* Wrap XLIB code in a new file as it defines keywords that conflict with Qt

    SPDX-FileCopyrightText: 2017 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.1-or-later
*/
#pragma once

typedef struct _XDisplay Display;

void sendXTestPressed(Display *display, int button);
void sendXTestReleased(Display *display, int button);
